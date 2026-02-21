#include "sandbox/voxel/meshing/controller.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "sandbox/logging.hpp"

namespace sandbox::voxel::meshing {

namespace {

[[nodiscard]] bool should_emit_face(
    world::BlockId current_block,
    world::BlockTraits current_traits,
    world::BlockId neighbor_block,
    world::BlockTraits neighbor_traits) {
    if (neighbor_traits.empty) {
        return true;
    }

    if (!world::is_full_cube(current_traits.shape) || !world::is_full_cube(neighbor_traits.shape)) {
        return true;
    }

    if (current_traits.render_layer == world::RenderLayer::translucent) {
        return neighbor_block != current_block;
    }

    return !(neighbor_traits.render_layer == world::RenderLayer::opaque
        && world::is_full_cube(neighbor_traits.shape));
}

struct ChunkBounds {
    float min_x = 0.0f;
    float min_y = 0.0f;
    float min_z = 0.0f;
    float max_x = 0.0f;
    float max_y = 0.0f;
    float max_z = 0.0f;
};

[[nodiscard]] ChunkBounds chunk_bounds(const world::ChunkKey& key) {
    const float chunk_extent = static_cast<float>(world::kChunkExtent);
    const float min_x = static_cast<float>(key.x) * chunk_extent;
    const float min_y = static_cast<float>(key.y) * chunk_extent;
    const float min_z = static_cast<float>(key.z) * chunk_extent;

    return ChunkBounds{
        .min_x = min_x,
        .min_y = min_y,
        .min_z = min_z,
        .max_x = min_x + chunk_extent,
        .max_y = min_y + chunk_extent,
        .max_z = min_z + chunk_extent,
    };
}

} // namespace

void Controller::initialize(const MeshingConfig& config) {
    config_ = config;

    {
        std::scoped_lock lock(worker_mutex_);
        build_queue_.clear();
        completed_queue_.clear();
        build_pending_set_.clear();
        upload_queue_.clear();
        uploaded_meshes_.clear();
        render_pass_buckets_ = RenderPassBuckets{};
        render_pass_stats_ = RenderPassStats{};
        workers_stopping_ = false;
    }

    initialized_ = true;
    start_workers();
}

void Controller::shutdown() {
    stop_workers();

    {
        std::scoped_lock lock(worker_mutex_);
        build_queue_.clear();
        completed_queue_.clear();
        build_pending_set_.clear();
        upload_queue_.clear();
        uploaded_meshes_.clear();
        render_pass_buckets_ = RenderPassBuckets{};
        render_pass_stats_ = RenderPassStats{};
    }

    initialized_ = false;
}

void Controller::update(world::World& world) {
    if (!initialized_) {
        return;
    }

    enqueue_dirty_chunks(world);
    process_completed_builds();
    process_upload_queue();
}

bool Controller::initialized() const {
    return initialized_;
}

std::size_t Controller::queued_build_count() const {
    std::scoped_lock lock(worker_mutex_);
    return build_queue_.size() + completed_queue_.size();
}

std::size_t Controller::queued_upload_count() const {
    std::scoped_lock lock(worker_mutex_);
    return upload_queue_.size();
}

std::size_t Controller::ready_mesh_count() const {
    std::scoped_lock lock(worker_mutex_);
    return uploaded_meshes_.size();
}

RenderPassBuckets Controller::render_pass_buckets() const {
    std::scoped_lock lock(worker_mutex_);
    return render_pass_buckets_;
}

RenderPassStats Controller::render_pass_stats() const {
    std::scoped_lock lock(worker_mutex_);
    return render_pass_stats_;
}

RenderPassBuckets Controller::visible_render_pass_buckets(const VisibilityQuery& query) const {
    RenderPassBuckets visible{};

    std::scoped_lock lock(worker_mutex_);
    visible.opaque_chunks.reserve(render_pass_buckets_.opaque_chunks.size());
    visible.cutout_chunks.reserve(render_pass_buckets_.cutout_chunks.size());
    visible.translucent_chunks.reserve(render_pass_buckets_.translucent_chunks.size());

    for (const world::ChunkKey& key : render_pass_buckets_.opaque_chunks) {
        if (is_chunk_visible(key, query)) {
            visible.opaque_chunks.push_back(key);
        }
    }
    for (const world::ChunkKey& key : render_pass_buckets_.cutout_chunks) {
        if (is_chunk_visible(key, query)) {
            visible.cutout_chunks.push_back(key);
        }
    }
    for (const world::ChunkKey& key : render_pass_buckets_.translucent_chunks) {
        if (is_chunk_visible(key, query)) {
            visible.translucent_chunks.push_back(key);
        }
    }

    return visible;
}

void Controller::enqueue_dirty_chunks(world::World& world) {
    const auto keys = world.chunk_keys();

    {
        std::scoped_lock lock(worker_mutex_);
        for (const world::ChunkKey& key : keys) {
            world::Chunk* chunk = world.find_chunk(key);
            if (chunk == nullptr || !chunk->dirty_mesh() || build_pending_set_.contains(key)) {
                continue;
            }

            BuildRequest request{};
            request.key = key;
            request.blocks = chunk->blocks();
            build_queue_.push_back(std::move(request));
            build_pending_set_.insert(key);
            chunk->clear_dirty_mesh();
        }
    }

    worker_cv_.notify_all();
}

void Controller::process_completed_builds() {
    std::vector<ChunkMeshInfo> staged_uploads;
    staged_uploads.reserve(config_.build_commit_budget_per_frame);

    {
        std::scoped_lock lock(worker_mutex_);
        const std::size_t process_count = std::min(config_.build_commit_budget_per_frame, completed_queue_.size());
        for (std::size_t index = 0; index < process_count; ++index) {
            staged_uploads.push_back(std::move(completed_queue_.front()));
            completed_queue_.pop_front();
        }
    }

    if (staged_uploads.empty()) {
        return;
    }

    {
        std::scoped_lock lock(worker_mutex_);
        for (ChunkMeshInfo& mesh : staged_uploads) {
            build_pending_set_.erase(mesh.key);
            upload_queue_.push_back(std::move(mesh));
        }
    }
}

void Controller::process_upload_queue() {
    std::scoped_lock lock(worker_mutex_);

    const std::size_t process_count = std::min(config_.upload_budget_per_frame, upload_queue_.size());
    bool any_uploads_committed = false;
    for (std::size_t index = 0; index < process_count; ++index) {
        ChunkMeshInfo mesh = std::move(upload_queue_.front());
        upload_queue_.pop_front();
        uploaded_meshes_[mesh.key] = mesh;
        any_uploads_committed = true;
    }

    if (any_uploads_committed) {
        rebuild_render_pass_buckets_locked();
    }
}

void Controller::rebuild_render_pass_buckets_locked() {
    render_pass_buckets_ = RenderPassBuckets{};
    render_pass_stats_ = RenderPassStats{};

    render_pass_buckets_.opaque_chunks.reserve(uploaded_meshes_.size());
    render_pass_buckets_.cutout_chunks.reserve(uploaded_meshes_.size());
    render_pass_buckets_.translucent_chunks.reserve(uploaded_meshes_.size());

    for (const auto& [key, mesh] : uploaded_meshes_) {
        if (mesh.opaque_face_count > 0) {
            render_pass_buckets_.opaque_chunks.push_back(key);
            ++render_pass_stats_.opaque_chunk_count;
            render_pass_stats_.opaque_face_count += mesh.opaque_face_count;
        }
        if (mesh.cutout_face_count > 0) {
            render_pass_buckets_.cutout_chunks.push_back(key);
            ++render_pass_stats_.cutout_chunk_count;
            render_pass_stats_.cutout_face_count += mesh.cutout_face_count;
        }
        if (mesh.translucent_face_count > 0) {
            render_pass_buckets_.translucent_chunks.push_back(key);
            ++render_pass_stats_.translucent_chunk_count;
            render_pass_stats_.translucent_face_count += mesh.translucent_face_count;
        }
    }
}

void Controller::start_workers() {
    if (!workers_.empty()) {
        stop_workers();
    }

    {
        std::scoped_lock lock(worker_mutex_);
        workers_stopping_ = false;
    }

    const std::size_t worker_count = std::max<std::size_t>(1, config_.workers);
    workers_.reserve(worker_count);
    for (std::size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
        workers_.emplace_back([this]() {
            worker_main();
        });
    }

    LOG_INFO("Meshing workers started: {}", workers_.size());
}

void Controller::stop_workers() {
    {
        std::scoped_lock lock(worker_mutex_);
        workers_stopping_ = true;
    }
    worker_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void Controller::worker_main() {
    while (true) {
        BuildRequest request{};

        {
            std::unique_lock lock(worker_mutex_);
            worker_cv_.wait(lock, [this]() {
                return workers_stopping_ || !build_queue_.empty();
            });

            if (workers_stopping_ && build_queue_.empty()) {
                return;
            }

            request = std::move(build_queue_.front());
            build_queue_.pop_front();
        }

        ChunkMeshInfo mesh = build_chunk_mesh(request);

        {
            std::scoped_lock lock(worker_mutex_);
            completed_queue_.push_back(std::move(mesh));
        }
    }
}

ChunkMeshInfo Controller::build_chunk_mesh(const BuildRequest& request) {
    auto block_at = [&request](int x, int y, int z) -> world::BlockId {
        const world::LocalVoxelCoord coord{x, y, z};
        return request.blocks[world::flatten_local_coord(coord)];
    };

    std::uint32_t solid_count = 0;
    std::uint32_t non_full_voxel_count = 0;
    std::uint32_t opaque_faces = 0;
    std::uint32_t cutout_faces = 0;
    std::uint32_t translucent_faces = 0;

    for (int z = 0; z < world::kChunkExtent; ++z) {
        for (int y = 0; y < world::kChunkExtent; ++y) {
            for (int x = 0; x < world::kChunkExtent; ++x) {
                const world::BlockId block_id = block_at(x, y, z);
                const world::BlockTraits traits = world::block_traits(block_id);
                if (traits.empty) {
                    continue;
                }

                ++solid_count;
                if (!world::is_full_cube(traits.shape)) {
                    ++non_full_voxel_count;
                }

                auto count_face = [&](bool visible) {
                    if (!visible) {
                        return;
                    }

                    switch (traits.render_layer) {
                        case world::RenderLayer::opaque:
                            ++opaque_faces;
                            break;
                        case world::RenderLayer::cutout:
                            ++cutout_faces;
                            break;
                        case world::RenderLayer::translucent:
                            ++translucent_faces;
                            break;
                    }
                };

                auto neighbor_traits = [&](int nx, int ny, int nz) {
                    const world::BlockId neighbor_id = block_at(nx, ny, nz);
                    return std::pair{neighbor_id, world::block_traits(neighbor_id)};
                };

                const bool pos_x_exposed = [&]() {
                    if (x == world::kChunkExtent - 1) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x + 1, y, z);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();
                const bool neg_x_exposed = [&]() {
                    if (x == 0) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x - 1, y, z);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();
                const bool pos_y_exposed = [&]() {
                    if (y == world::kChunkExtent - 1) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x, y + 1, z);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();
                const bool neg_y_exposed = [&]() {
                    if (y == 0) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x, y - 1, z);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();
                const bool pos_z_exposed = [&]() {
                    if (z == world::kChunkExtent - 1) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x, y, z + 1);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();
                const bool neg_z_exposed = [&]() {
                    if (z == 0) {
                        return true;
                    }
                    const auto [neighbor_id, neighbor] = neighbor_traits(x, y, z - 1);
                    return should_emit_face(block_id, traits, neighbor_id, neighbor);
                }();

                count_face(pos_x_exposed);
                count_face(neg_x_exposed);
                count_face(pos_y_exposed);
                count_face(neg_y_exposed);
                count_face(pos_z_exposed);
                count_face(neg_z_exposed);
            }
        }
    }

    const std::uint32_t total_exposed_faces = opaque_faces + cutout_faces + translucent_faces;

    return ChunkMeshInfo{
        .key = request.key,
        .solid_voxel_count = solid_count,
        .non_full_voxel_count = non_full_voxel_count,
        .opaque_face_count = opaque_faces,
        .cutout_face_count = cutout_faces,
        .translucent_face_count = translucent_faces,
        .total_exposed_face_count = total_exposed_faces,
    };
}

bool Controller::is_chunk_visible(const world::ChunkKey& key, const VisibilityQuery& query) {
    if (query.enable_distance_cull) {
        const world::ChunkKey origin_chunk = world::world_to_chunk_key(query.origin_world);
        const int dx = std::abs(key.x - origin_chunk.x);
        const int dy = std::abs(key.y - origin_chunk.y);
        const int dz = std::abs(key.z - origin_chunk.z);

        if (dx > query.max_chunk_distance || dy > query.max_chunk_distance || dz > query.max_chunk_distance) {
            return false;
        }
    }

    if (!query.enable_frustum_cull) {
        return true;
    }

    const ChunkBounds bounds = chunk_bounds(key);
    for (const FrustumPlane& plane : query.frustum_planes) {
        const float px = plane.nx >= 0.0f ? bounds.max_x : bounds.min_x;
        const float py = plane.ny >= 0.0f ? bounds.max_y : bounds.min_y;
        const float pz = plane.nz >= 0.0f ? bounds.max_z : bounds.min_z;

        const float distance = plane.nx * px + plane.ny * py + plane.nz * pz + plane.d;
        if (distance < 0.0f) {
            return false;
        }
    }

    return true;
}

} // namespace sandbox::voxel::meshing
