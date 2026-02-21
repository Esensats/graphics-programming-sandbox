#include "sandbox/voxel/meshing/controller.hpp"

#include <algorithm>

#include "sandbox/logging.hpp"

namespace sandbox::voxel::meshing {

void Controller::initialize(const MeshingConfig& config) {
    config_ = config;

    {
        std::scoped_lock lock(worker_mutex_);
        build_queue_.clear();
        completed_queue_.clear();
        build_pending_set_.clear();
        upload_queue_.clear();
        uploaded_meshes_.clear();
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
    for (std::size_t index = 0; index < process_count; ++index) {
        ChunkMeshInfo mesh = std::move(upload_queue_.front());
        upload_queue_.pop_front();
        uploaded_meshes_[mesh.key] = mesh;
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
    std::uint32_t exposed_faces = 0;

    for (int z = 0; z < world::kChunkExtent; ++z) {
        for (int y = 0; y < world::kChunkExtent; ++y) {
            for (int x = 0; x < world::kChunkExtent; ++x) {
                if (block_at(x, y, z) == world::kAirBlockId) {
                    continue;
                }

                ++solid_count;

                const bool pos_x_exposed = x == world::kChunkExtent - 1 || block_at(x + 1, y, z) == world::kAirBlockId;
                const bool neg_x_exposed = x == 0 || block_at(x - 1, y, z) == world::kAirBlockId;
                const bool pos_y_exposed = y == world::kChunkExtent - 1 || block_at(x, y + 1, z) == world::kAirBlockId;
                const bool neg_y_exposed = y == 0 || block_at(x, y - 1, z) == world::kAirBlockId;
                const bool pos_z_exposed = z == world::kChunkExtent - 1 || block_at(x, y, z + 1) == world::kAirBlockId;
                const bool neg_z_exposed = z == 0 || block_at(x, y, z - 1) == world::kAirBlockId;

                exposed_faces += static_cast<std::uint32_t>(pos_x_exposed);
                exposed_faces += static_cast<std::uint32_t>(neg_x_exposed);
                exposed_faces += static_cast<std::uint32_t>(pos_y_exposed);
                exposed_faces += static_cast<std::uint32_t>(neg_y_exposed);
                exposed_faces += static_cast<std::uint32_t>(pos_z_exposed);
                exposed_faces += static_cast<std::uint32_t>(neg_z_exposed);
            }
        }
    }

    return ChunkMeshInfo{
        .key = request.key,
        .solid_voxel_count = solid_count,
        .exposed_face_count = exposed_faces,
    };
}

} // namespace sandbox::voxel::meshing
