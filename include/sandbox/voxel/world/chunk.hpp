#pragma once

#include <array>

#include "sandbox/voxel/world/block.hpp"
#include "sandbox/voxel/world/chunk_coords.hpp"

namespace sandbox::voxel::world {

class Chunk {
  public:
    Chunk() {
        blocks_.fill(kAirBlockId);
    }

    [[nodiscard]] BlockId get(LocalVoxelCoord coord) const {
        return blocks_[flatten_local_coord(coord)];
    }

    [[nodiscard]] bool set(LocalVoxelCoord coord, BlockId block_id) {
        const std::size_t index = flatten_local_coord(coord);
        if (blocks_[index] == block_id) {
            return false;
        }

        blocks_[index] = block_id;
        dirty_mesh_ = true;
        return true;
    }

    [[nodiscard]] const std::array<BlockId, kChunkVolume>& blocks() const {
        return blocks_;
    }

    [[nodiscard]] bool dirty_mesh() const {
        return dirty_mesh_;
    }

    void clear_dirty_mesh() {
        dirty_mesh_ = false;
    }

    void mark_dirty_mesh() {
        dirty_mesh_ = true;
    }

  private:
    std::array<BlockId, kChunkVolume> blocks_{};
    bool dirty_mesh_ = true;
};

} // namespace sandbox::voxel::world
