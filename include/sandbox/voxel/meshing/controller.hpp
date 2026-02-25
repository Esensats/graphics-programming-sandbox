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

#include "sandbox/graphics/mesh_utils.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel::meshing {

struct MeshingConfig {
    std::size_t workers = 1;
    std::size_t build_commit_budget_per_frame = 16;
    std::size_t upload_budget_per_frame = 16;
};

struct ChunkMeshInfo {
  struct SurfaceMesh {
    std::vector<float> vertices{};
    std::vector<unsigned int> indices{};
  };

    world::ChunkKey key{};
    std::uint32_t solid_voxel_count = 0;
  std::uint32_t non_full_voxel_count = 0;
  std::uint32_t opaque_face_count = 0;
  std::uint32_t cutout_face_count = 0;
  std::uint32_t translucent_face_count = 0;
  std::uint32_t total_exposed_face_count = 0;

  SurfaceMesh opaque_mesh{};
  SurfaceMesh cutout_mesh{};
  SurfaceMesh translucent_mesh{};
};

struct DrawCommand {
  unsigned int vao = 0;
  int index_count = 0;
  float sort_center_x = 0.0f;
  float sort_center_y = 0.0f;
  float sort_center_z = 0.0f;
};

struct VisibleDrawLists {
  std::vector<DrawCommand> opaque{};
  std::vector<DrawCommand> cutout{};
  std::vector<DrawCommand> translucent{};
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

struct UploadedMeshMeta {
  std::uint32_t opaque_face_count = 0;
  std::uint32_t cutout_face_count = 0;
  std::uint32_t translucent_face_count = 0;
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
    struct BuildRequest {
        world::ChunkKey key{};
        std::array<world::BlockId, world::kChunkVolume> blocks{};
        std::array<bool, 6> neighbor_face_present{};
        std::array<std::array<world::BlockId, world::kChunkArea>, 6> neighbor_face_blocks{};
    };

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
    [[nodiscard]] VisibleDrawLists visible_draw_lists(const VisibilityQuery& query) const;

    [[nodiscard]] static ChunkMeshInfo build_chunk_mesh(const BuildRequest& request);

  private:
    void enqueue_dirty_chunks(world::World& world);
    void process_completed_builds();
    void process_upload_queue();
    void rebuild_render_pass_buckets_locked();

    void start_workers();
    void stop_workers();
    void worker_main();

    [[nodiscard]] static bool is_chunk_visible(const world::ChunkKey& key, const VisibilityQuery& query);

    struct ChunkGpuMesh {
      graphics::IndexedMeshHandles opaque{};
      graphics::IndexedMeshHandles cutout{};
      graphics::IndexedMeshHandles translucent{};
    };

    static void destroy_chunk_gpu_mesh(ChunkGpuMesh& mesh);
    static bool upload_surface_to_gpu(const ChunkMeshInfo::SurfaceMesh& surface,
                      graphics::IndexedMeshHandles& handles);

    void prune_unloaded_chunks_locked(const world::World& world);

    MeshingConfig config_{};
    bool initialized_ = false;

    std::deque<BuildRequest> build_queue_{};
    std::deque<ChunkMeshInfo> completed_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> build_pending_set_{};

    std::deque<ChunkMeshInfo> upload_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> upload_pending_set_{};
    std::unordered_map<world::ChunkKey, UploadedMeshMeta, world::ChunkKeyHash> uploaded_mesh_meta_{};
    std::unordered_map<world::ChunkKey, ChunkGpuMesh, world::ChunkKeyHash> gpu_meshes_{};
    RenderPassBuckets render_pass_buckets_{};
    RenderPassStats render_pass_stats_{};

    mutable std::mutex worker_mutex_{};
    std::condition_variable worker_cv_{};
    bool workers_stopping_ = false;
    std::vector<std::thread> workers_{};
};

} // namespace sandbox::voxel::meshing
