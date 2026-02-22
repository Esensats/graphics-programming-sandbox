#include "sandbox/voxel/streaming/residency.hpp"

#include <algorithm>
#include <cstdlib>

#include "sandbox/logging.hpp"

namespace sandbox::voxel::streaming {

void ResidencyController::initialize(const StreamingConfig& config) {
    config_ = config;
    generator_.set_seed(config_.seed);

    desired_chunks_.clear();
    desired_set_.clear();

    {
        std::scoped_lock lock(generation_mutex_);
        generation_queue_.clear();
        generation_pending_set_.clear();
        generated_chunk_queue_.clear();
        workers_stopping_ = false;
    }

    unload_queue_.clear();
    unload_queued_set_.clear();

    focus_world_ = world::WorldVoxelCoord{};
    focus_chunk_ = world::world_to_chunk_key(focus_world_);
    initialized_ = true;
    start_workers();
}

void ResidencyController::shutdown() {
    stop_workers();

    desired_chunks_.clear();
    desired_set_.clear();

    {
        std::scoped_lock lock(generation_mutex_);
        generation_queue_.clear();
        generation_pending_set_.clear();
        generated_chunk_queue_.clear();
    }

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
    process_generated_chunks(world);
    process_unload_jobs(world);
}

void ResidencyController::regenerate_loaded_chunks(world::World& world) {
    if (!initialized_) {
        return;
    }

    const std::vector<world::ChunkKey> keys = world.chunk_keys();
    for (const world::ChunkKey& key : keys) {
        world::Chunk* chunk = world.find_chunk(key);
        if (chunk == nullptr) {
            continue;
        }

        generator_.populate_chunk(key, *chunk);
        chunk->mark_dirty_mesh();
    }
}

bool ResidencyController::initialized() const {
    return initialized_;
}

std::size_t ResidencyController::queued_generation_count() const {
    std::scoped_lock lock(generation_mutex_);
    return generation_queue_.size() + generated_chunk_queue_.size();
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
    std::scoped_lock lock(generation_mutex_);

    for (const world::ChunkKey& key : desired_chunks_) {
        if (world.has_chunk(key) || generation_pending_set_.contains(key)) {
            continue;
        }

        generation_queue_.push_back(key);
        generation_pending_set_.insert(key);
    }

    generation_cv_.notify_all();
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

void ResidencyController::process_generated_chunks(world::World& world) {
    std::vector<GeneratedChunk> pending_commits;
    pending_commits.reserve(config_.generation_budget_per_frame);

    {
        std::scoped_lock lock(generation_mutex_);
        const std::size_t process_count = std::min(config_.generation_budget_per_frame, generated_chunk_queue_.size());
        for (std::size_t index = 0; index < process_count; ++index) {
            pending_commits.push_back(std::move(generated_chunk_queue_.front()));
            generated_chunk_queue_.pop_front();
        }
    }

    for (GeneratedChunk& generated : pending_commits) {
        bool should_commit = false;
        {
            std::scoped_lock lock(generation_mutex_);
            generation_pending_set_.erase(generated.key);
            should_commit = desired_set_.contains(generated.key);
        }

        if (!should_commit) {
            continue;
        }

        world::Chunk& target = world.ensure_chunk(generated.key);
        target = std::move(generated.chunk);
        target.mark_dirty_mesh();
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

void ResidencyController::start_workers() {
    if (!workers_.empty()) {
        stop_workers();
    }

    {
        std::scoped_lock lock(generation_mutex_);
        workers_stopping_ = false;
    }

    const std::size_t worker_count = std::max<std::size_t>(1, config_.generation_workers);
    workers_.reserve(worker_count);
    for (std::size_t worker_index = 0; worker_index < worker_count; ++worker_index) {
        workers_.emplace_back([this]() {
            worker_main();
        });
    }

    LOG_INFO("Streaming generation workers started: {}", workers_.size());
}

void ResidencyController::stop_workers() {
    {
        std::scoped_lock lock(generation_mutex_);
        workers_stopping_ = true;
        generation_queue_.clear();
        generation_pending_set_.clear();
    }
    generation_cv_.notify_all();

    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();
}

void ResidencyController::worker_main() {
    while (true) {
        world::ChunkKey key{};

        {
            std::unique_lock lock(generation_mutex_);
            generation_cv_.wait(lock, [this]() {
                return workers_stopping_ || !generation_queue_.empty();
            });

            if (workers_stopping_ && generation_queue_.empty()) {
                return;
            }

            key = generation_queue_.front();
            generation_queue_.pop_front();
        }

        GeneratedChunk generated{};
        generated.key = key;
        generator_.populate_chunk(key, generated.chunk);

        {
            std::scoped_lock lock(generation_mutex_);
            if (workers_stopping_) {
                continue;
            }
            generated_chunk_queue_.push_back(std::move(generated));
        }
    }
}

} // namespace sandbox::voxel::streaming
