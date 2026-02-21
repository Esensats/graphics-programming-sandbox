#include "sandbox/voxel/runtime.hpp"

#include "sandbox/logging.hpp"

namespace sandbox::voxel {

void Runtime::initialize() {
    if (initialized_) {
        return;
    }

    world_.clear();
    residency_.initialize(streaming::StreamingConfig{
        .horizontal_radius_chunks = 2,
        .vertical_radius_chunks = 1,
        .generation_budget_per_frame = 12,
        .unload_budget_per_frame = 12,
        .generation_workers = 2,
        .seed = 1337,
    });
    meshing_.initialize(meshing::MeshingConfig{
        .workers = 2,
        .build_commit_budget_per_frame = 24,
        .upload_budget_per_frame = 24,
    });
    residency_.set_focus_world(world::WorldVoxelCoord{0, 0, 0});
    residency_.update(world_);
    meshing_.update(world_);

    simulation_time_seconds_ = 0.0;
    initialized_ = true;
    LOG_INFO("Voxel runtime initialized (active chunks: {}, generation queued: {}, upload queued: {})",
        world_.active_chunk_count(),
        residency_.queued_generation_count(),
        meshing_.queued_upload_count());
}

void Runtime::shutdown() {
    if (!initialized_) {
        return;
    }

    meshing_.shutdown();
    residency_.shutdown();
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
    residency_.set_focus_world(world::WorldVoxelCoord{0, 0, 0});
    residency_.update(world_);
    meshing_.update(world_);
}

} // namespace sandbox::voxel
