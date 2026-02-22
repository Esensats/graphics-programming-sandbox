#pragma once

#include <cstddef>

#include "sandbox/voxel/meshing/controller.hpp"
#include "sandbox/voxel/streaming/residency.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel {

struct RuntimeDebugSnapshot {
    std::size_t active_chunk_count = 0;
    std::size_t generation_queued_count = 0;
    std::size_t upload_queued_count = 0;
};

class Runtime {
  public:
    void initialize();
    void shutdown();

    void update_fixed(float step_seconds);
    void update_frame(float delta_seconds);

    void debug_mark_all_chunks_dirty_mesh();
    void debug_regenerate_loaded_chunks();

    [[nodiscard]] meshing::RenderPassBuckets visible_render_pass_buckets(const meshing::VisibilityQuery& query) const;
    [[nodiscard]] meshing::VisibleDrawLists visible_draw_lists(const meshing::VisibilityQuery& query) const;
    [[nodiscard]] RuntimeDebugSnapshot debug_snapshot() const;

    [[nodiscard]] bool initialized() const {
        return initialized_;
    }

  private:
    bool initialized_ = false;
    double simulation_time_seconds_ = 0.0;
    world::World world_{};
    streaming::ResidencyController residency_{};
    meshing::Controller meshing_{};
};

} // namespace sandbox::voxel
