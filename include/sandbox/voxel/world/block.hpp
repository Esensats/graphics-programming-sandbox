#pragma once

#include <cstdint>

namespace sandbox::voxel::world {

using BlockId = std::uint16_t;
using MaterialId = std::uint16_t;

inline constexpr BlockId kAirBlockId = 0;
inline constexpr MaterialId kDefaultMaterialId = 0;

struct BlockDefinition {
    BlockId id = kAirBlockId;
    MaterialId material_id = kDefaultMaterialId;
    bool solid = false;
};

} // namespace sandbox::voxel::world
