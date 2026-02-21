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
    const meshing::RenderPassStats pass_stats = meshing_.render_pass_stats();
    const meshing::RenderPassBuckets visible_buckets = meshing_.visible_render_pass_buckets(meshing::VisibilityQuery{
        .origin_world = world::WorldVoxelCoord{0, 0, 0},
        .enable_distance_cull = true,
        .max_chunk_distance = 2,
        .enable_frustum_cull = false,
    });

    simulation_time_seconds_ = 0.0;
    initialized_ = true;
    LOG_INFO("Voxel runtime initialized (active chunks: {}, generation queued: {}, upload queued: {}, opaque chunks: {}, cutout chunks: {}, translucent chunks: {}, visible opaque: {}, visible cutout: {}, visible translucent: {})",
        world_.active_chunk_count(),
        residency_.queued_generation_count(),
        meshing_.queued_upload_count(),
        pass_stats.opaque_chunk_count,
        pass_stats.cutout_chunk_count,
        pass_stats.translucent_chunk_count,
        visible_buckets.opaque_chunks.size(),
        visible_buckets.cutout_chunks.size(),
        visible_buckets.translucent_chunks.size());
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

meshing::RenderPassBuckets Runtime::visible_render_pass_buckets(const meshing::VisibilityQuery& query) const {
    return meshing_.visible_render_pass_buckets(query);
}

} // namespace sandbox::voxel
