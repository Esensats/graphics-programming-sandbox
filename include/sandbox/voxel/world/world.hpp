#pragma once

#include <array>
#include <optional>
#include <unordered_map>

#include "sandbox/voxel/world/chunk.hpp"

namespace sandbox::voxel::world {

enum class SetBlockResult {
    missing_chunk,
    unchanged,
    updated,
};

class World {
  public:
    [[nodiscard]] bool has_chunk(const ChunkKey& key) const;
    [[nodiscard]] std::size_t active_chunk_count() const;

    void clear();
    Chunk& ensure_chunk(const ChunkKey& key);
    bool erase_chunk(const ChunkKey& key);

    [[nodiscard]] Chunk* find_chunk(const ChunkKey& key);
    [[nodiscard]] const Chunk* find_chunk(const ChunkKey& key) const;

    [[nodiscard]] std::optional<BlockId> try_get_block(WorldVoxelCoord coord) const;
    [[nodiscard]] BlockId get_block_or(WorldVoxelCoord coord, BlockId fallback) const;
    [[nodiscard]] SetBlockResult set_block(WorldVoxelCoord coord, BlockId block_id);

    [[nodiscard]] std::array<std::optional<BlockId>, 6> neighbor_blocks(WorldVoxelCoord coord) const;

  private:
    std::unordered_map<ChunkKey, Chunk, ChunkKeyHash> chunks_{};
};

} // namespace sandbox::voxel::world
