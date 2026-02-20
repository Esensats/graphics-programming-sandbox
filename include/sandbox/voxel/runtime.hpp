#pragma once

#include "sandbox/voxel/streaming/residency.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel {

class Runtime {
  public:
    void initialize();
    void shutdown();

    void update_fixed(float step_seconds);
    void update_frame(float delta_seconds);

    [[nodiscard]] bool initialized() const {
        return initialized_;
    }

  private:
    bool initialized_ = false;
    double simulation_time_seconds_ = 0.0;
    world::World world_{};
    streaming::ResidencyController residency_{};
};

} // namespace sandbox::voxel
