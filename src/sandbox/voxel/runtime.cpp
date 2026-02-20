#include "sandbox/voxel/runtime.hpp"

#include "sandbox/logging.hpp"

namespace sandbox::voxel {

void Runtime::initialize() {
    if (initialized_) {
        return;
    }

    world_.clear();
    world_.ensure_chunk(world::ChunkKey{0, 0, 0});
    (void)world_.set_block(world::WorldVoxelCoord{0, 0, 0}, 1);

    simulation_time_seconds_ = 0.0;
    initialized_ = true;
    LOG_INFO("Voxel runtime initialized (active chunks: {})", world_.active_chunk_count());
}

void Runtime::shutdown() {
    if (!initialized_) {
        return;
    }

    world_.clear();
    initialized_ = false;
    simulation_time_seconds_ = 0.0;
    LOG_INFO("Voxel runtime shutdown");
}

void Runtime::update_fixed(float step_seconds) {
    if (!initialized_) {
        return;
    }

    simulation_time_seconds_ += static_cast<double>(step_seconds);
}

void Runtime::update_frame(float delta_seconds) {
    if (!initialized_) {
        return;
    }

    (void)delta_seconds;
}

} // namespace sandbox::voxel
