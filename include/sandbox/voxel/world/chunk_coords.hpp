#pragma once

#include <array>
#include <cstddef>

namespace sandbox::voxel::world {

inline constexpr int kChunkExtent = 32;
inline constexpr int kChunkArea = kChunkExtent * kChunkExtent;
inline constexpr int kChunkVolume = kChunkExtent * kChunkExtent * kChunkExtent;

struct ChunkKey {
    int x = 0;
    int y = 0;
    int z = 0;

    [[nodiscard]] bool operator==(const ChunkKey& other) const = default;
};

struct ChunkKeyHash {
    [[nodiscard]] std::size_t operator()(const ChunkKey& key) const noexcept {
        const std::size_t hx = static_cast<std::size_t>(key.x) * 73856093u;
        const std::size_t hy = static_cast<std::size_t>(key.y) * 19349663u;
        const std::size_t hz = static_cast<std::size_t>(key.z) * 83492791u;
        return hx ^ hy ^ hz;
    }
};

struct LocalVoxelCoord {
    int x = 0;
    int y = 0;
    int z = 0;
};

struct WorldVoxelCoord {
    int x = 0;
    int y = 0;
    int z = 0;
};

[[nodiscard]] constexpr int floor_div(int value, int divisor) noexcept {
    const int quotient = value / divisor;
    const int remainder = value % divisor;
    if (remainder != 0 && ((remainder > 0) != (divisor > 0))) {
        return quotient - 1;
    }
    return quotient;
}

[[nodiscard]] constexpr int positive_mod(int value, int divisor) noexcept {
    const int remainder = value % divisor;
    if (remainder < 0) {
        return remainder + divisor;
    }
    return remainder;
}

[[nodiscard]] constexpr bool is_local_coord_in_bounds(LocalVoxelCoord coord) noexcept {
    return coord.x >= 0 && coord.y >= 0 && coord.z >= 0
        && coord.x < kChunkExtent && coord.y < kChunkExtent && coord.z < kChunkExtent;
}

[[nodiscard]] constexpr std::size_t flatten_local_coord(LocalVoxelCoord coord) noexcept {
    return static_cast<std::size_t>(coord.x)
        + static_cast<std::size_t>(coord.y) * static_cast<std::size_t>(kChunkExtent)
        + static_cast<std::size_t>(coord.z) * static_cast<std::size_t>(kChunkArea);
}

[[nodiscard]] constexpr ChunkKey world_to_chunk_key(WorldVoxelCoord coord) noexcept {
    return {
        floor_div(coord.x, kChunkExtent),
        floor_div(coord.y, kChunkExtent),
        floor_div(coord.z, kChunkExtent),
    };
}

[[nodiscard]] constexpr LocalVoxelCoord world_to_local_coord(WorldVoxelCoord coord) noexcept {
    return {
        positive_mod(coord.x, kChunkExtent),
        positive_mod(coord.y, kChunkExtent),
        positive_mod(coord.z, kChunkExtent),
    };
}

[[nodiscard]] constexpr WorldVoxelCoord chunk_and_local_to_world(ChunkKey chunk_key, LocalVoxelCoord local_coord) noexcept {
    return {
        chunk_key.x * kChunkExtent + local_coord.x,
        chunk_key.y * kChunkExtent + local_coord.y,
        chunk_key.z * kChunkExtent + local_coord.z,
    };
}

[[nodiscard]] constexpr std::array<WorldVoxelCoord, 6> face_neighbors(WorldVoxelCoord coord) noexcept {
    return {
        WorldVoxelCoord{coord.x + 1, coord.y, coord.z},
        WorldVoxelCoord{coord.x - 1, coord.y, coord.z},
        WorldVoxelCoord{coord.x, coord.y + 1, coord.z},
        WorldVoxelCoord{coord.x, coord.y - 1, coord.z},
        WorldVoxelCoord{coord.x, coord.y, coord.z + 1},
        WorldVoxelCoord{coord.x, coord.y, coord.z - 1},
    };
}

} // namespace sandbox::voxel::world
