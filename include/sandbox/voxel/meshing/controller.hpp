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
  std::uint32_t non_full_voxel_count = 0;
  std::uint32_t opaque_face_count = 0;
  std::uint32_t cutout_face_count = 0;
  std::uint32_t translucent_face_count = 0;
  std::uint32_t total_exposed_face_count = 0;
};

struct RenderPassBuckets {
  std::vector<world::ChunkKey> opaque_chunks{};
  std::vector<world::ChunkKey> cutout_chunks{};
  std::vector<world::ChunkKey> translucent_chunks{};
};

struct RenderPassStats {
  std::size_t opaque_chunk_count = 0;
  std::size_t cutout_chunk_count = 0;
  std::size_t translucent_chunk_count = 0;
  std::uint64_t opaque_face_count = 0;
  std::uint64_t cutout_face_count = 0;
  std::uint64_t translucent_face_count = 0;
};

struct FrustumPlane {
  float nx = 0.0f;
  float ny = 0.0f;
  float nz = 0.0f;
  float d = 0.0f;
};

struct VisibilityQuery {
  world::WorldVoxelCoord origin_world{};
  bool enable_distance_cull = true;
  int max_chunk_distance = 8;
  bool enable_frustum_cull = false;
  std::array<FrustumPlane, 6> frustum_planes{};
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
    [[nodiscard]] RenderPassBuckets render_pass_buckets() const;
    [[nodiscard]] RenderPassStats render_pass_stats() const;
    [[nodiscard]] RenderPassBuckets visible_render_pass_buckets(const VisibilityQuery& query) const;

  private:
    struct BuildRequest {
        world::ChunkKey key{};
        std::array<world::BlockId, world::kChunkVolume> blocks{};
    };

    void enqueue_dirty_chunks(world::World& world);
    void process_completed_builds();
    void process_upload_queue();
    void rebuild_render_pass_buckets_locked();

    void start_workers();
    void stop_workers();
    void worker_main();

    [[nodiscard]] static ChunkMeshInfo build_chunk_mesh(const BuildRequest& request);
    [[nodiscard]] static bool is_chunk_visible(const world::ChunkKey& key, const VisibilityQuery& query);

    MeshingConfig config_{};
    bool initialized_ = false;

    std::deque<BuildRequest> build_queue_{};
    std::deque<ChunkMeshInfo> completed_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> build_pending_set_{};

    std::deque<ChunkMeshInfo> upload_queue_{};
    std::unordered_map<world::ChunkKey, ChunkMeshInfo, world::ChunkKeyHash> uploaded_meshes_{};
    RenderPassBuckets render_pass_buckets_{};
    RenderPassStats render_pass_stats_{};

    mutable std::mutex worker_mutex_{};
    std::condition_variable worker_cv_{};
    bool workers_stopping_ = false;
    std::vector<std::thread> workers_{};
};

} // namespace sandbox::voxel::meshing
