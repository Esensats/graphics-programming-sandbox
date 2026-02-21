#pragma once

#include <cstdint>

namespace sandbox::voxel::world {

using BlockId = std::uint16_t;
using MaterialId = std::uint16_t;

inline constexpr BlockId kAirBlockId = 0;
inline constexpr BlockId kStoneBlockId = 1;
inline constexpr BlockId kGlassBlockId = 2;
inline constexpr BlockId kWaterBlockId = 3;
inline constexpr BlockId kSlabBlockId = 4;
inline constexpr BlockId kGrassBlockId = 5;

inline constexpr MaterialId kDefaultMaterialId = 0;

enum class RenderLayer {
    opaque,
    cutout,
    translucent,
};

enum class BlockShape {
    full_cube,
    slab,
    cross,
    liquid,
};

struct BlockTraits {
    bool empty = false;
    bool collision_solid = false;
    RenderLayer render_layer = RenderLayer::opaque;
    BlockShape shape = BlockShape::full_cube;
};

struct BlockDefinition {
    BlockId id = kAirBlockId;
    MaterialId material_id = kDefaultMaterialId;
    bool solid = false;
};

[[nodiscard]] constexpr float block_material_layer(BlockId block_id) {
    switch (block_id) {
        case kAirBlockId:
            return 0.0f;
        case kStoneBlockId:
            return 1.0f;
        case kSlabBlockId:
            return 2.0f;
        case kGrassBlockId:
            return 3.0f;
        case kWaterBlockId:
            return 4.0f;
        case kGlassBlockId:
            return 5.0f;
        default:
            return 1.0f;
    }
}

[[nodiscard]] constexpr bool is_full_cube(BlockShape shape) {
    return shape == BlockShape::full_cube;
}

[[nodiscard]] constexpr BlockTraits block_traits(BlockId block_id) {
    switch (block_id) {
        case kAirBlockId:
            return BlockTraits{.empty = true, .collision_solid = false, .render_layer = RenderLayer::opaque, .shape = BlockShape::full_cube};
        case kStoneBlockId:
            return BlockTraits{.empty = false, .collision_solid = true, .render_layer = RenderLayer::opaque, .shape = BlockShape::full_cube};
        case kGlassBlockId:
            return BlockTraits{.empty = false, .collision_solid = true, .render_layer = RenderLayer::translucent, .shape = BlockShape::full_cube};
        case kWaterBlockId:
            return BlockTraits{.empty = false, .collision_solid = false, .render_layer = RenderLayer::translucent, .shape = BlockShape::liquid};
        case kSlabBlockId:
            return BlockTraits{.empty = false, .collision_solid = true, .render_layer = RenderLayer::opaque, .shape = BlockShape::slab};
        case kGrassBlockId:
            return BlockTraits{.empty = false, .collision_solid = false, .render_layer = RenderLayer::cutout, .shape = BlockShape::cross};
        default:
            return BlockTraits{.empty = false, .collision_solid = true, .render_layer = RenderLayer::opaque, .shape = BlockShape::full_cube};
    }
}

} // namespace sandbox::voxel::world
