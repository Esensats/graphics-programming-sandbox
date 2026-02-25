#include "sandbox/voxel/meshing/controller.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <utility>

#include "sandbox/logging.hpp"

namespace sandbox::voxel::meshing {

namespace {

constexpr std::array<graphics::VertexAttribute, 3> k_position_uv_layer_attributes{{
    {0, 3, 6, 0},
    {1, 2, 6, 3},
    {2, 1, 6, 5},
}};

constexpr int k_face_pos_x = 0;
constexpr int k_face_neg_x = 1;
constexpr int k_face_pos_y = 2;
constexpr int k_face_neg_y = 3;
constexpr int k_face_pos_z = 4;
constexpr int k_face_neg_z = 5;

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
        return neighbor_traits.render_layer == world::RenderLayer::translucent
            && neighbor_block != current_block;
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

[[nodiscard]] std::array<float, 3> chunk_center(const world::ChunkKey& key) {
    const float chunk_extent = static_cast<float>(world::kChunkExtent);
    const float half_extent = chunk_extent * 0.5f;
    return {
        static_cast<float>(key.x) * chunk_extent + half_extent,
        static_cast<float>(key.y) * chunk_extent + half_extent,
        static_cast<float>(key.z) * chunk_extent + half_extent,
    };
}

[[nodiscard]] world::ChunkKey neighbor_chunk_key(const world::ChunkKey& key, int face_index) {
    switch (face_index) {
        case k_face_pos_x:
            return world::ChunkKey{key.x + 1, key.y, key.z};
        case k_face_neg_x:
            return world::ChunkKey{key.x - 1, key.y, key.z};
        case k_face_pos_y:
            return world::ChunkKey{key.x, key.y + 1, key.z};
        case k_face_neg_y:
            return world::ChunkKey{key.x, key.y - 1, key.z};
        case k_face_pos_z:
            return world::ChunkKey{key.x, key.y, key.z + 1};
        case k_face_neg_z:
            return world::ChunkKey{key.x, key.y, key.z - 1};
        default:
            return key;
    }
}

[[nodiscard]] std::size_t face_slice_index(int u, int v) {
    return static_cast<std::size_t>(u)
        + static_cast<std::size_t>(v) * static_cast<std::size_t>(world::kChunkExtent);
}

/**
 * Fills the neighbor face block ID slice for the given face index from the provided neighbor chunk.
 * Marks the neighbor face as present in the build request.
 */
void fill_neighbor_face_slice(Controller::BuildRequest& request,
                              int face_index,
                              const world::Chunk& neighbor_chunk) {
    const auto& blocks = neighbor_chunk.blocks();
    auto& face_blocks = request.neighbor_face_blocks[static_cast<std::size_t>(face_index)];

    for (int v = 0; v < world::kChunkExtent; ++v) {
        for (int u = 0; u < world::kChunkExtent; ++u) {
            world::LocalVoxelCoord neighbor_coord{};
            switch (face_index) {
                case k_face_pos_x:
                    neighbor_coord = world::LocalVoxelCoord{0, u, v};
                    break;
                case k_face_neg_x:
                    neighbor_coord = world::LocalVoxelCoord{world::kChunkExtent - 1, u, v};
                    break;
                case k_face_pos_y:
                    neighbor_coord = world::LocalVoxelCoord{u, 0, v};
                    break;
                case k_face_neg_y:
                    neighbor_coord = world::LocalVoxelCoord{u, world::kChunkExtent - 1, v};
                    break;
                case k_face_pos_z:
                    neighbor_coord = world::LocalVoxelCoord{u, v, 0};
                    break;
                case k_face_neg_z:
                    neighbor_coord = world::LocalVoxelCoord{u, v, world::kChunkExtent - 1};
                    break;
                default:
                    neighbor_coord = world::LocalVoxelCoord{0, 0, 0};
                    break;
            }

            const std::size_t src_index = world::flatten_local_coord(neighbor_coord);
            face_blocks[face_slice_index(u, v)] = blocks[src_index];
        }
    }

    request.neighbor_face_present[static_cast<std::size_t>(face_index)] = true;
}

[[nodiscard]] world::BlockId sample_block_with_neighbor_faces(const Controller::BuildRequest& request,
                                                              int x,
                                                              int y,
                                                              int z) {
    if (x >= 0 && x < world::kChunkExtent
        && y >= 0 && y < world::kChunkExtent
        && z >= 0 && z < world::kChunkExtent) {
        return request.blocks[world::flatten_local_coord(world::LocalVoxelCoord{x, y, z})];
    }

    if (x == world::kChunkExtent) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_pos_x)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_pos_x)][face_slice_index(y, z)];
    }

    if (x == -1) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_neg_x)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_neg_x)][face_slice_index(y, z)];
    }

    if (y == world::kChunkExtent) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_pos_y)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_pos_y)][face_slice_index(x, z)];
    }

    if (y == -1) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_neg_y)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_neg_y)][face_slice_index(x, z)];
    }

    if (z == world::kChunkExtent) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_pos_z)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_pos_z)][face_slice_index(x, y)];
    }

    if (z == -1) {
        if (!request.neighbor_face_present[static_cast<std::size_t>(k_face_neg_z)]) {
            return world::kAirBlockId;
        }
        return request.neighbor_face_blocks[static_cast<std::size_t>(k_face_neg_z)][face_slice_index(x, y)];
    }

    return world::kAirBlockId;
}

void append_face(ChunkMeshInfo::SurfaceMesh& surface,
                 const std::array<std::array<float, 3>, 4>& positions,
                 float material_layer) {
    const unsigned int base_index = static_cast<unsigned int>(surface.vertices.size() / 6);

    constexpr std::array<std::array<float, 2>, 4> uv{{
        {0.0f, 0.0f},
        {0.0f, 1.0f},
        {1.0f, 1.0f},
        {1.0f, 0.0f},
    }};

    for (std::size_t index = 0; index < positions.size(); ++index) {
        const auto& pos = positions[index];
        surface.vertices.push_back(pos[0]);
        surface.vertices.push_back(pos[1]);
        surface.vertices.push_back(pos[2]);
        surface.vertices.push_back(uv[index][0]);
        surface.vertices.push_back(uv[index][1]);
        surface.vertices.push_back(material_layer);
    }

    surface.indices.push_back(base_index + 0);
    surface.indices.push_back(base_index + 1);
    surface.indices.push_back(base_index + 2);
    surface.indices.push_back(base_index + 2);
    surface.indices.push_back(base_index + 3);
    surface.indices.push_back(base_index + 0);
}

void append_voxel_face(ChunkMeshInfo::SurfaceMesh& surface,
                       float material_layer,
                       float x,
                       float y,
                       float z,
                       int face_index) {
    switch (face_index) {
        case 0:
            append_face(surface, {{{x + 1.0f, y + 0.0f, z + 0.0f},
                                  {x + 1.0f, y + 1.0f, z + 0.0f},
                                  {x + 1.0f, y + 1.0f, z + 1.0f},
                                  {x + 1.0f, y + 0.0f, z + 1.0f}}}, material_layer);
            break;
        case 1:
            append_face(surface, {{{x + 0.0f, y + 0.0f, z + 1.0f},
                                  {x + 0.0f, y + 1.0f, z + 1.0f},
                                  {x + 0.0f, y + 1.0f, z + 0.0f},
                                  {x + 0.0f, y + 0.0f, z + 0.0f}}}, material_layer);
            break;
        case 2:
            append_face(surface, {{{x + 0.0f, y + 1.0f, z + 0.0f},
                                  {x + 0.0f, y + 1.0f, z + 1.0f},
                                  {x + 1.0f, y + 1.0f, z + 1.0f},
                                  {x + 1.0f, y + 1.0f, z + 0.0f}}}, material_layer);
            break;
        case 3:
            append_face(surface, {{{x + 0.0f, y + 0.0f, z + 1.0f},
                                  {x + 0.0f, y + 0.0f, z + 0.0f},
                                  {x + 1.0f, y + 0.0f, z + 0.0f},
                                  {x + 1.0f, y + 0.0f, z + 1.0f}}}, material_layer);
            break;
        case 4:
            append_face(surface, {{{x + 1.0f, y + 0.0f, z + 1.0f},
                                  {x + 1.0f, y + 1.0f, z + 1.0f},
                                  {x + 0.0f, y + 1.0f, z + 1.0f},
                                  {x + 0.0f, y + 0.0f, z + 1.0f}}}, material_layer);
            break;
        case 5:
            append_face(surface, {{{x + 0.0f, y + 0.0f, z + 0.0f},
                                  {x + 0.0f, y + 1.0f, z + 0.0f},
                                  {x + 1.0f, y + 1.0f, z + 0.0f},
                                  {x + 1.0f, y + 0.0f, z + 0.0f}}}, material_layer);
            break;
        default:
            break;
    }
}

ChunkMeshInfo::SurfaceMesh& mesh_for_layer(ChunkMeshInfo& mesh_info, world::RenderLayer layer) {
    switch (layer) {
        case world::RenderLayer::opaque:
            return mesh_info.opaque_mesh;
        case world::RenderLayer::cutout:
            return mesh_info.cutout_mesh;
        case world::RenderLayer::translucent:
            return mesh_info.translucent_mesh;
    }

    return mesh_info.opaque_mesh;
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
        upload_pending_set_.clear();

        for (auto& [key, gpu] : gpu_meshes_) {
            (void)key;
            destroy_chunk_gpu_mesh(gpu);
        }

        uploaded_mesh_meta_.clear();
        gpu_meshes_.clear();
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
        upload_pending_set_.clear();

        for (auto& [key, gpu] : gpu_meshes_) {
            (void)key;
            destroy_chunk_gpu_mesh(gpu);
        }

        uploaded_mesh_meta_.clear();
        gpu_meshes_.clear();
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
    return uploaded_mesh_meta_.size();
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

VisibleDrawLists Controller::visible_draw_lists(const VisibilityQuery& query) const {
    VisibleDrawLists draws{};

    std::scoped_lock lock(worker_mutex_);
    draws.opaque.reserve(render_pass_buckets_.opaque_chunks.size());
    draws.cutout.reserve(render_pass_buckets_.cutout_chunks.size());
    draws.translucent.reserve(render_pass_buckets_.translucent_chunks.size());

    auto push_for_keys = [this, &query](const std::vector<world::ChunkKey>& keys,
                                        std::vector<DrawCommand>& out,
                                        auto get_handles) {
        for (const world::ChunkKey& key : keys) {
            if (!is_chunk_visible(key, query)) {
                continue;
            }

            const auto it = gpu_meshes_.find(key);
            if (it == gpu_meshes_.end()) {
                continue;
            }

            const graphics::IndexedMeshHandles& handles = get_handles(it->second);
            if (handles.vao == 0 || handles.index_count <= 0) {
                continue;
            }

            const auto center = chunk_center(key);

            out.push_back(DrawCommand{
                .vao = handles.vao,
                .index_count = handles.index_count,
                .sort_center_x = center[0],
                .sort_center_y = center[1],
                .sort_center_z = center[2],
            });
        }
    };

    push_for_keys(render_pass_buckets_.opaque_chunks, draws.opaque, [](const ChunkGpuMesh& mesh) -> const graphics::IndexedMeshHandles& {
        return mesh.opaque;
    });
    push_for_keys(render_pass_buckets_.cutout_chunks, draws.cutout, [](const ChunkGpuMesh& mesh) -> const graphics::IndexedMeshHandles& {
        return mesh.cutout;
    });
    push_for_keys(render_pass_buckets_.translucent_chunks, draws.translucent, [](const ChunkGpuMesh& mesh) -> const graphics::IndexedMeshHandles& {
        return mesh.translucent;
    });

    return draws;
}

void Controller::enqueue_dirty_chunks(world::World& world) {
    {
        std::scoped_lock lock(worker_mutex_);
        prune_unloaded_chunks_locked(world);
    }

    const auto keys = world.chunk_keys();

    for (const world::ChunkKey& key : keys) {
        world::Chunk* chunk = world.find_chunk(key);
        if (chunk == nullptr || !chunk->dirty_mesh()) {
            continue;
        }

        for (int face = 0; face < 6; ++face) {
            const world::ChunkKey neighbor_key = neighbor_chunk_key(key, face);
            world::Chunk* neighbor_chunk = world.find_chunk(neighbor_key);
            if (neighbor_chunk == nullptr) {
                continue;
            }

            neighbor_chunk->mark_dirty_mesh();
        }
    }

    {
        std::scoped_lock lock(worker_mutex_);
        for (const world::ChunkKey& key : keys) {
            world::Chunk* chunk = world.find_chunk(key);
            if (chunk == nullptr || !chunk->dirty_mesh() || build_pending_set_.contains(key) || upload_pending_set_.contains(key)) {
                continue;
            }

            BuildRequest request{};
            request.key = key;
            request.blocks = chunk->blocks();

            for (int face = 0; face < 6; ++face) {
                const world::ChunkKey neighbor_key = neighbor_chunk_key(key, face);
                const world::Chunk* neighbor_chunk = world.find_chunk(neighbor_key);
                if (neighbor_chunk == nullptr) {
                    continue;
                }

                fill_neighbor_face_slice(request, face, *neighbor_chunk);
            }

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
            const world::ChunkKey key = mesh.key;
            if (upload_pending_set_.contains(key)) {
                auto queued = std::find_if(upload_queue_.begin(), upload_queue_.end(), [&key](const ChunkMeshInfo& queued_mesh) {
                    return queued_mesh.key == key;
                });
                if (queued != upload_queue_.end()) {
                    *queued = std::move(mesh);
                }
            } else {
                upload_pending_set_.insert(key);
                upload_queue_.push_back(std::move(mesh));
            }
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
        upload_pending_set_.erase(mesh.key);

        ChunkGpuMesh& gpu = gpu_meshes_[mesh.key];
        destroy_chunk_gpu_mesh(gpu);

        if (!upload_surface_to_gpu(mesh.opaque_mesh, gpu.opaque)) {
            graphics::destroy_indexed_mesh(gpu.opaque);
        }
        if (!upload_surface_to_gpu(mesh.cutout_mesh, gpu.cutout)) {
            graphics::destroy_indexed_mesh(gpu.cutout);
        }
        if (!upload_surface_to_gpu(mesh.translucent_mesh, gpu.translucent)) {
            graphics::destroy_indexed_mesh(gpu.translucent);
        }

        uploaded_mesh_meta_[mesh.key] = UploadedMeshMeta{
            .opaque_face_count = mesh.opaque_face_count,
            .cutout_face_count = mesh.cutout_face_count,
            .translucent_face_count = mesh.translucent_face_count,
        };
        any_uploads_committed = true;
    }

    if (any_uploads_committed) {
        rebuild_render_pass_buckets_locked();
    }
}

void Controller::rebuild_render_pass_buckets_locked() {
    render_pass_buckets_ = RenderPassBuckets{};
    render_pass_stats_ = RenderPassStats{};

    render_pass_buckets_.opaque_chunks.reserve(uploaded_mesh_meta_.size());
    render_pass_buckets_.cutout_chunks.reserve(uploaded_mesh_meta_.size());
    render_pass_buckets_.translucent_chunks.reserve(uploaded_mesh_meta_.size());

    for (const auto& [key, meta] : uploaded_mesh_meta_) {
        if (meta.opaque_face_count > 0) {
            render_pass_buckets_.opaque_chunks.push_back(key);
            ++render_pass_stats_.opaque_chunk_count;
            render_pass_stats_.opaque_face_count += meta.opaque_face_count;
        }
        if (meta.cutout_face_count > 0) {
            render_pass_buckets_.cutout_chunks.push_back(key);
            ++render_pass_stats_.cutout_chunk_count;
            render_pass_stats_.cutout_face_count += meta.cutout_face_count;
        }
        if (meta.translucent_face_count > 0) {
            render_pass_buckets_.translucent_chunks.push_back(key);
            ++render_pass_stats_.translucent_chunk_count;
            render_pass_stats_.translucent_face_count += meta.translucent_face_count;
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
        build_queue_.clear();
        build_pending_set_.clear();
        upload_queue_.clear();
        upload_pending_set_.clear();
        completed_queue_.clear();
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
            if (workers_stopping_) {
                continue;
            }
            completed_queue_.push_back(std::move(mesh));
        }
    }
}

ChunkMeshInfo Controller::build_chunk_mesh(const BuildRequest& request) {
    auto block_at = [&request](int x, int y, int z) -> world::BlockId {
        return sample_block_with_neighbor_faces(request, x, y, z);
    };

    ChunkMeshInfo mesh_info{};
    mesh_info.key = request.key;

    for (int z = 0; z < world::kChunkExtent; ++z) {
        for (int y = 0; y < world::kChunkExtent; ++y) {
            for (int x = 0; x < world::kChunkExtent; ++x) {
                const world::BlockId block_id = block_at(x, y, z);
                const float material_layer = world::block_material_layer(block_id);
                const world::BlockTraits traits = world::block_traits(block_id);
                if (traits.empty) {
                    continue;
                }

                ++mesh_info.solid_voxel_count;
                if (!world::is_full_cube(traits.shape)) {
                    ++mesh_info.non_full_voxel_count;
                }

                auto count_face = [&](bool visible) {
                    if (!visible) {
                        return;
                    }

                    switch (traits.render_layer) {
                        case world::RenderLayer::opaque:
                            ++mesh_info.opaque_face_count;
                            break;
                        case world::RenderLayer::cutout:
                            ++mesh_info.cutout_face_count;
                            break;
                        case world::RenderLayer::translucent:
                            ++mesh_info.translucent_face_count;
                            break;
                    }
                };

                auto neighbor_traits = [&](int nx, int ny, int nz) {
                    const world::BlockId neighbor_id = block_at(nx, ny, nz);
                    return std::pair{neighbor_id, world::block_traits(neighbor_id)};
                };

                const bool face_visible[6] = {
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x + 1, y, z);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x - 1, y, z);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x, y + 1, z);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x, y - 1, z);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x, y, z + 1);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                    [&]() {
                        const auto [neighbor_id, neighbor] = neighbor_traits(x, y, z - 1);
                        return should_emit_face(block_id, traits, neighbor_id, neighbor);
                    }(),
                };

                for (int face = 0; face < 6; ++face) {
                    count_face(face_visible[face]);
                    if (!face_visible[face]) {
                        continue;
                    }

                    const float world_x = static_cast<float>(request.key.x * world::kChunkExtent + x);
                    const float world_y = static_cast<float>(request.key.y * world::kChunkExtent + y);
                    const float world_z = static_cast<float>(request.key.z * world::kChunkExtent + z);

                    ChunkMeshInfo::SurfaceMesh& surface = mesh_for_layer(mesh_info, traits.render_layer);
                    append_voxel_face(surface,
                                      material_layer,
                                      world_x,
                                      world_y,
                                      world_z,
                                      face);
                }
            }
        }
    }

    mesh_info.total_exposed_face_count =
        mesh_info.opaque_face_count + mesh_info.cutout_face_count + mesh_info.translucent_face_count;

    return mesh_info;
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

void Controller::destroy_chunk_gpu_mesh(ChunkGpuMesh& mesh) {
    graphics::destroy_indexed_mesh(mesh.opaque);
    graphics::destroy_indexed_mesh(mesh.cutout);
    graphics::destroy_indexed_mesh(mesh.translucent);
}

bool Controller::upload_surface_to_gpu(const ChunkMeshInfo::SurfaceMesh& surface,
                                       graphics::IndexedMeshHandles& handles) {
    if (surface.vertices.empty() || surface.indices.empty()) {
        return false;
    }

    return graphics::create_indexed_mesh(surface.vertices, surface.indices, k_position_uv_layer_attributes, handles);
}

void Controller::prune_unloaded_chunks_locked(const world::World& world) {
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> loaded_keys;
    for (const world::ChunkKey& key : world.chunk_keys()) {
        loaded_keys.insert(key);
    }

    std::vector<world::ChunkKey> remove_keys;
    remove_keys.reserve(uploaded_mesh_meta_.size());

    for (const auto& [key, meta] : uploaded_mesh_meta_) {
        (void)meta;
        if (!loaded_keys.contains(key)) {
            remove_keys.push_back(key);
        }
    }

    for (const world::ChunkKey& key : remove_keys) {
        uploaded_mesh_meta_.erase(key);
        auto gpu_it = gpu_meshes_.find(key);
        if (gpu_it != gpu_meshes_.end()) {
            destroy_chunk_gpu_mesh(gpu_it->second);
            gpu_meshes_.erase(gpu_it);
        }

        auto upload_it = std::find_if(upload_queue_.begin(), upload_queue_.end(), [&key](const ChunkMeshInfo& queued_mesh) {
            return queued_mesh.key == key;
        });
        if (upload_it != upload_queue_.end()) {
            upload_queue_.erase(upload_it);
        }
        upload_pending_set_.erase(key);
    }

    if (!remove_keys.empty()) {
        rebuild_render_pass_buckets_locked();
    }
}

} // namespace sandbox::voxel::meshing
