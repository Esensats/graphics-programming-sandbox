#include "sandbox/voxel/runtime.hpp"

#include "sandbox/logging.hpp"

namespace sandbox::voxel {

void Runtime::initialize() {
    initialize(RuntimeConfig{
        .streaming = streaming::StreamingConfig{
            .horizontal_radius_chunks = 2,
            .vertical_radius_chunks = 1,
            .generation_budget_per_frame = 12,
            .unload_budget_per_frame = 12,
            .generation_workers = 2,
            .seed = 1337,
        },
        .meshing = meshing::MeshingConfig{
            .workers = 2,
            .build_commit_budget_per_frame = 24,
            .upload_budget_per_frame = 24,
        },
    });
}

void Runtime::initialize(const RuntimeConfig& config) {
    if (initialized_) {
        return;
    }

    world_.clear();
    residency_.initialize(config.streaming);
    meshing_.initialize(config.meshing);
    residency_.set_focus_world(world::WorldVoxelCoord{0, 0, 0});
    residency_.update(world_);
    meshing_.update(world_);
    // TODO: This is not even needed ata all. What's the purpose of `visible_render_pass_buckets`?
    // const meshing::RenderPassStats pass_stats = meshing_.render_pass_stats();
    // const meshing::RenderPassBuckets visible_buckets = meshing_.visible_render_pass_buckets(meshing::VisibilityQuery{
    //     .origin_world = world::WorldVoxelCoord{0, 0, 0},
    //     .enable_distance_cull = true,
    //     .max_chunk_distance = 2,
    //     .enable_frustum_cull = false,
    // });

    simulation_time_seconds_ = 0.0;
    initialized_ = true;
    LOG_INFO("Voxel runtime initialized (active chunks: {}, generation queued: {}, upload queued: {})",
        world_.active_chunk_count(),
        residency_.queued_generation_count(),
        meshing_.queued_upload_count());
    // LOG_INFO("Voxel runtime initialized (active chunks: {}, generation queued: {}, upload queued: {}, opaque chunks: {}, cutout chunks: {}, translucent chunks: {}, visible opaque: {}, visible cutout: {}, visible translucent: {})",
    //     world_.active_chunk_count(),
    //     residency_.queued_generation_count(),
    //     meshing_.queued_upload_count(),
    //     pass_stats.opaque_chunk_count,
    //     pass_stats.cutout_chunk_count,
    //     pass_stats.translucent_chunk_count,
    //     visible_buckets.opaque_chunks.size(),
    //     visible_buckets.cutout_chunks.size(),
    //     visible_buckets.translucent_chunks.size());
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

void Runtime::debug_mark_all_chunks_dirty_mesh() {
    if (!initialized_) {
        return;
    }

    const std::vector<world::ChunkKey> keys = world_.chunk_keys();
    for (const world::ChunkKey& key : keys) {
        world::Chunk* chunk = world_.find_chunk(key);
        if (chunk == nullptr) {
            continue;
        }

        chunk->mark_dirty_mesh();
    }

    LOG_INFO("Voxel runtime debug remesh requested for {} chunks", keys.size());
}

void Runtime::debug_regenerate_loaded_chunks() {
    if (!initialized_) {
        return;
    }

    const std::size_t chunk_count = world_.active_chunk_count();
    residency_.regenerate_loaded_chunks(world_);
    LOG_INFO("Voxel runtime debug regenerate+remesh requested for {} chunks", chunk_count);
}

meshing::RenderPassBuckets Runtime::visible_render_pass_buckets(const meshing::VisibilityQuery& query) const {
    return meshing_.visible_render_pass_buckets(query);
}

meshing::VisibleDrawLists Runtime::visible_draw_lists(const meshing::VisibilityQuery& query) const {
    return meshing_.visible_draw_lists(query);
}

RuntimeDebugSnapshot Runtime::debug_snapshot() const {
    return RuntimeDebugSnapshot{
        .active_chunk_count = world_.active_chunk_count(),
        .generation_queued_count = residency_.queued_generation_count(),
        .upload_queued_count = meshing_.queued_upload_count(),
    };
}

} // namespace sandbox::voxel
