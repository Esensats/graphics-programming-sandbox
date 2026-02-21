#pragma once

namespace sandbox::voxel::render {

struct MaterialPack {
    unsigned int albedo_array = 0;
    int layer_count = 0;
    int layer_resolution = 0;
};

MaterialPack create_placeholder_material_pack();
MaterialPack create_material_pack_from_directory(const char* directory_path);
void destroy_material_pack(MaterialPack& pack);

} // namespace sandbox::voxel::render
