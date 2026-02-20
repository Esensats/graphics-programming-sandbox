#include "sandbox/voxel/world/world.hpp"

namespace sandbox::voxel::world {

bool World::has_chunk(const ChunkKey& key) const {
    return chunks_.contains(key);
}

std::size_t World::active_chunk_count() const {
    return chunks_.size();
}

std::vector<ChunkKey> World::chunk_keys() const {
    std::vector<ChunkKey> keys;
    keys.reserve(chunks_.size());

    for (const auto& [key, chunk] : chunks_) {
        (void)chunk;
        keys.push_back(key);
    }

    return keys;
}

void World::clear() {
    chunks_.clear();
}

Chunk& World::ensure_chunk(const ChunkKey& key) {
    auto [it, inserted] = chunks_.try_emplace(key);
    if (inserted) {
        it->second.mark_dirty_mesh();
    }
    return it->second;
}

bool World::erase_chunk(const ChunkKey& key) {
    return chunks_.erase(key) != 0;
}

Chunk* World::find_chunk(const ChunkKey& key) {
    const auto it = chunks_.find(key);
    if (it == chunks_.end()) {
        return nullptr;
    }
    return &it->second;
}

const Chunk* World::find_chunk(const ChunkKey& key) const {
    const auto it = chunks_.find(key);
    if (it == chunks_.end()) {
        return nullptr;
    }
    return &it->second;
}

std::optional<BlockId> World::try_get_block(WorldVoxelCoord coord) const {
    const ChunkKey chunk_key = world_to_chunk_key(coord);
    const LocalVoxelCoord local_coord = world_to_local_coord(coord);

    const Chunk* chunk = find_chunk(chunk_key);
    if (chunk == nullptr) {
        return std::nullopt;
    }

    return chunk->get(local_coord);
}

BlockId World::get_block_or(WorldVoxelCoord coord, BlockId fallback) const {
    const std::optional<BlockId> block_id = try_get_block(coord);
    if (!block_id.has_value()) {
        return fallback;
    }
    return *block_id;
}

SetBlockResult World::set_block(WorldVoxelCoord coord, BlockId block_id) {
    const ChunkKey chunk_key = world_to_chunk_key(coord);
    const LocalVoxelCoord local_coord = world_to_local_coord(coord);

    Chunk* chunk = find_chunk(chunk_key);
    if (chunk == nullptr) {
        return SetBlockResult::missing_chunk;
    }

    if (!chunk->set(local_coord, block_id)) {
        return SetBlockResult::unchanged;
    }

    return SetBlockResult::updated;
}

std::array<std::optional<BlockId>, 6> World::neighbor_blocks(WorldVoxelCoord coord) const {
    std::array<std::optional<BlockId>, 6> neighbors{};

    const auto neighbor_coords = face_neighbors(coord);
    for (std::size_t index = 0; index < neighbor_coords.size(); ++index) {
        neighbors[index] = try_get_block(neighbor_coords[index]);
    }

    return neighbors;
}

} // namespace sandbox::voxel::world
