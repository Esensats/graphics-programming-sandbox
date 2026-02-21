#pragma once

#include <array>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel::meshing {

struct MeshingConfig {
    std::size_t workers = 1;
    std::size_t build_commit_budget_per_frame = 16;
    std::size_t upload_budget_per_frame = 16;
};

struct ChunkMeshInfo {
    world::ChunkKey key{};
    std::uint32_t solid_voxel_count = 0;
    std::uint32_t exposed_face_count = 0;
};

class Controller {
  public:
    void initialize(const MeshingConfig& config);
    void shutdown();

    void update(world::World& world);

    [[nodiscard]] bool initialized() const;
    [[nodiscard]] std::size_t queued_build_count() const;
    [[nodiscard]] std::size_t queued_upload_count() const;
    [[nodiscard]] std::size_t ready_mesh_count() const;

  private:
    struct BuildRequest {
        world::ChunkKey key{};
        std::array<world::BlockId, world::kChunkVolume> blocks{};
    };

    void enqueue_dirty_chunks(world::World& world);
    void process_completed_builds();
    void process_upload_queue();

    void start_workers();
    void stop_workers();
    void worker_main();

    [[nodiscard]] static ChunkMeshInfo build_chunk_mesh(const BuildRequest& request);

    MeshingConfig config_{};
    bool initialized_ = false;

    std::deque<BuildRequest> build_queue_{};
    std::deque<ChunkMeshInfo> completed_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> build_pending_set_{};

    std::deque<ChunkMeshInfo> upload_queue_{};
    std::unordered_map<world::ChunkKey, ChunkMeshInfo, world::ChunkKeyHash> uploaded_meshes_{};

    mutable std::mutex worker_mutex_{};
    std::condition_variable worker_cv_{};
    bool workers_stopping_ = false;
    std::vector<std::thread> workers_{};
};

} // namespace sandbox::voxel::meshing
