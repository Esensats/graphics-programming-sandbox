#include "sandbox/voxel/streaming/terrain_generator.hpp"

#include <algorithm>
#include <cstdint>

namespace sandbox::voxel::streaming {

namespace {

[[nodiscard]] std::uint32_t hash_mix(std::uint32_t value) {
    value ^= value >> 16u;
    value *= 0x7feb352du;
    value ^= value >> 15u;
    value *= 0x846ca68bu;
    value ^= value >> 16u;
    return value;
}

} // namespace

TerrainGenerator::TerrainGenerator(std::uint32_t seed)
    : seed_(seed) {
}

void TerrainGenerator::set_seed(std::uint32_t seed) {
    seed_ = seed;
}

std::uint32_t TerrainGenerator::seed() const {
    return seed_;
}

int TerrainGenerator::sample_height(int world_x, int world_z) const {
    const std::uint32_t x = static_cast<std::uint32_t>(world_x);
    const std::uint32_t z = static_cast<std::uint32_t>(world_z);
    const std::uint32_t mixed = hash_mix(seed_ ^ (x * 0x9e3779b9u) ^ (z * 0x85ebca6bu));

    const int noise = static_cast<int>(mixed & 0x0Fu);
    return 8 + noise;
}

void TerrainGenerator::populate_chunk(const world::ChunkKey& key, world::Chunk& chunk) const {
    for (int local_z = 0; local_z < world::kChunkExtent; ++local_z) {
        for (int local_y = 0; local_y < world::kChunkExtent; ++local_y) {
            for (int local_x = 0; local_x < world::kChunkExtent; ++local_x) {
                const world::LocalVoxelCoord local_coord{local_x, local_y, local_z};
                const world::WorldVoxelCoord world_coord = world::chunk_and_local_to_world(key, local_coord);
                const int terrain_height = sample_height(world_coord.x, world_coord.z);

                const world::BlockId block_id = world_coord.y <= terrain_height
                    ? static_cast<world::BlockId>(1)
                    : world::kAirBlockId;
                (void)chunk.set(local_coord, block_id);
            }
        }
    }

    chunk.mark_dirty_mesh();
}

} // namespace sandbox::voxel::streaming
