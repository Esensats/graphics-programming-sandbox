#pragma once

#include "sandbox/voxel/meshing/controller.hpp"
#include "sandbox/voxel/streaming/residency.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel {

class Runtime {
  public:
    void initialize();
    void shutdown();

    void update_fixed(float step_seconds);
    void update_frame(float delta_seconds);

    [[nodiscard]] meshing::RenderPassBuckets visible_render_pass_buckets(const meshing::VisibilityQuery& query) const;

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
