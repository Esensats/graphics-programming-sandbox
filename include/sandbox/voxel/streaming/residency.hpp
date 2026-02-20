#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <unordered_set>
#include <vector>

#include "sandbox/voxel/streaming/terrain_generator.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel::streaming {

struct StreamingConfig {
    int horizontal_radius_chunks = 4;
    int vertical_radius_chunks = 2;
    std::size_t generation_budget_per_frame = 8;
    std::size_t unload_budget_per_frame = 8;
    std::uint32_t seed = 1;
};

class ResidencyController {
  public:
    void initialize(const StreamingConfig& config);
    void shutdown();

    void set_focus_world(world::WorldVoxelCoord focus_coord);
    void update(world::World& world);

    [[nodiscard]] bool initialized() const;
    [[nodiscard]] std::size_t queued_generation_count() const;
    [[nodiscard]] std::size_t queued_unload_count() const;

  private:
    void rebuild_desired_set();
    void enqueue_generation_jobs(const world::World& world);
    void enqueue_unload_jobs(const world::World& world);
    void process_generation_jobs(world::World& world);
    void process_unload_jobs(world::World& world);

    [[nodiscard]] bool is_within_focus_range(const world::ChunkKey& key) const;

    StreamingConfig config_{};
    TerrainGenerator generator_{};
    bool initialized_ = false;

    world::WorldVoxelCoord focus_world_{};
    world::ChunkKey focus_chunk_{};

    std::vector<world::ChunkKey> desired_chunks_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> desired_set_{};

    std::deque<world::ChunkKey> generation_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> generation_queued_set_{};

    std::deque<world::ChunkKey> unload_queue_{};
    std::unordered_set<world::ChunkKey, world::ChunkKeyHash> unload_queued_set_{};
};

} // namespace sandbox::voxel::streaming
