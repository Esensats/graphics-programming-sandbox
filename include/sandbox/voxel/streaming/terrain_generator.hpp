#pragma once

#include <cstdint>

#include "sandbox/voxel/world/chunk.hpp"

namespace sandbox::voxel::streaming {

class TerrainGenerator {
  public:
    explicit TerrainGenerator(std::uint32_t seed = 0);

    void set_seed(std::uint32_t seed);
    [[nodiscard]] std::uint32_t seed() const;

    void populate_chunk(const world::ChunkKey& key, world::Chunk& chunk) const;

  private:
    [[nodiscard]] int sample_height(int world_x, int world_z) const;

    std::uint32_t seed_ = 0;
};

} // namespace sandbox::voxel::streaming
