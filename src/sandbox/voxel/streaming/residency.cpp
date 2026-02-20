#include "sandbox/voxel/streaming/residency.hpp"

#include <algorithm>
#include <cstdlib>

namespace sandbox::voxel::streaming {

void ResidencyController::initialize(const StreamingConfig& config) {
    config_ = config;
    generator_.set_seed(config_.seed);

    desired_chunks_.clear();
    desired_set_.clear();

    generation_queue_.clear();
    generation_queued_set_.clear();

    unload_queue_.clear();
    unload_queued_set_.clear();

    focus_world_ = world::WorldVoxelCoord{};
    focus_chunk_ = world::world_to_chunk_key(focus_world_);
    initialized_ = true;
}

void ResidencyController::shutdown() {
    desired_chunks_.clear();
    desired_set_.clear();

    generation_queue_.clear();
    generation_queued_set_.clear();

    unload_queue_.clear();
    unload_queued_set_.clear();

    initialized_ = false;
}

void ResidencyController::set_focus_world(world::WorldVoxelCoord focus_coord) {
    focus_world_ = focus_coord;
    focus_chunk_ = world::world_to_chunk_key(focus_world_);
}

void ResidencyController::update(world::World& world) {
    if (!initialized_) {
        return;
    }

    rebuild_desired_set();
    enqueue_generation_jobs(world);
    enqueue_unload_jobs(world);
    process_generation_jobs(world);
    process_unload_jobs(world);
}

bool ResidencyController::initialized() const {
    return initialized_;
}

std::size_t ResidencyController::queued_generation_count() const {
    return generation_queue_.size();
}

std::size_t ResidencyController::queued_unload_count() const {
    return unload_queue_.size();
}

void ResidencyController::rebuild_desired_set() {
    desired_chunks_.clear();
    desired_set_.clear();

    const int min_x = focus_chunk_.x - config_.horizontal_radius_chunks;
    const int max_x = focus_chunk_.x + config_.horizontal_radius_chunks;
    const int min_y = focus_chunk_.y - config_.vertical_radius_chunks;
    const int max_y = focus_chunk_.y + config_.vertical_radius_chunks;
    const int min_z = focus_chunk_.z - config_.horizontal_radius_chunks;
    const int max_z = focus_chunk_.z + config_.horizontal_radius_chunks;

    desired_chunks_.reserve(static_cast<std::size_t>(max_x - min_x + 1)
        * static_cast<std::size_t>(max_y - min_y + 1)
        * static_cast<std::size_t>(max_z - min_z + 1));

    for (int z = min_z; z <= max_z; ++z) {
        for (int y = min_y; y <= max_y; ++y) {
            for (int x = min_x; x <= max_x; ++x) {
                const world::ChunkKey key{x, y, z};
                desired_chunks_.push_back(key);
                desired_set_.insert(key);
            }
        }
    }
}

void ResidencyController::enqueue_generation_jobs(const world::World& world) {
    for (const world::ChunkKey& key : desired_chunks_) {
        if (world.has_chunk(key) || generation_queued_set_.contains(key)) {
            continue;
        }

        generation_queue_.push_back(key);
        generation_queued_set_.insert(key);
    }
}

void ResidencyController::enqueue_unload_jobs(const world::World& world) {
    const auto loaded_keys = world.chunk_keys();
    for (const world::ChunkKey& key : loaded_keys) {
        if (is_within_focus_range(key) || unload_queued_set_.contains(key)) {
            continue;
        }

        unload_queue_.push_back(key);
        unload_queued_set_.insert(key);
    }
}

void ResidencyController::process_generation_jobs(world::World& world) {
    const std::size_t process_count = std::min(config_.generation_budget_per_frame, generation_queue_.size());
    for (std::size_t index = 0; index < process_count; ++index) {
        const world::ChunkKey key = generation_queue_.front();
        generation_queue_.pop_front();
        generation_queued_set_.erase(key);

        world::Chunk& chunk = world.ensure_chunk(key);
        generator_.populate_chunk(key, chunk);
    }
}

void ResidencyController::process_unload_jobs(world::World& world) {
    const std::size_t process_count = std::min(config_.unload_budget_per_frame, unload_queue_.size());
    for (std::size_t index = 0; index < process_count; ++index) {
        const world::ChunkKey key = unload_queue_.front();
        unload_queue_.pop_front();
        unload_queued_set_.erase(key);

        if (!is_within_focus_range(key)) {
            (void)world.erase_chunk(key);
        }
    }
}

bool ResidencyController::is_within_focus_range(const world::ChunkKey& key) const {
    const int dx = std::abs(key.x - focus_chunk_.x);
    const int dy = std::abs(key.y - focus_chunk_.y);
    const int dz = std::abs(key.z - focus_chunk_.z);

    return dx <= config_.horizontal_radius_chunks
        && dy <= config_.vertical_radius_chunks
        && dz <= config_.horizontal_radius_chunks;
}

} // namespace sandbox::voxel::streaming
