#include "sandbox/voxel/render/material_pack.hpp"

#include <array>
#include <vector>

#include <glad/gl.h>

namespace sandbox::voxel::render {
namespace {

constexpr int k_layer_resolution = 16;
constexpr int k_layer_count = 6;

void fill_checker(std::vector<unsigned char>& pixels,
                  int resolution,
                  const std::array<unsigned char, 3>& color_a,
                  const std::array<unsigned char, 3>& color_b,
                  int checker_size) {
    pixels.resize(static_cast<std::size_t>(resolution * resolution * 4), 255);

    for (int y = 0; y < resolution; ++y) {
        for (int x = 0; x < resolution; ++x) {
            const bool toggle = ((x / checker_size) + (y / checker_size)) % 2 == 0;
            const auto& color = toggle ? color_a : color_b;
            const std::size_t index = static_cast<std::size_t>((y * resolution + x) * 4);
            pixels[index + 0] = color[0];
            pixels[index + 1] = color[1];
            pixels[index + 2] = color[2];
            pixels[index + 3] = 255;
        }
    }
}

} // namespace

MaterialPack create_placeholder_material_pack() {
    MaterialPack pack{};

    unsigned int texture = 0;
    glGenTextures(1, &texture);
    if (texture == 0) {
        return pack;
    }

    glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_REPEAT);

    glTexImage3D(GL_TEXTURE_2D_ARRAY,
                 0,
                 GL_RGBA8,
                 k_layer_resolution,
                 k_layer_resolution,
                 k_layer_count,
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);

    std::vector<unsigned char> layer_pixels;

    fill_checker(layer_pixels, k_layer_resolution, {0, 0, 0}, {0, 0, 0}, 4);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    fill_checker(layer_pixels, k_layer_resolution, {110, 110, 115}, {95, 95, 100}, 4);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 1, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    fill_checker(layer_pixels, k_layer_resolution, {135, 185, 95}, {120, 165, 85}, 4);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 2, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    fill_checker(layer_pixels, k_layer_resolution, {165, 225, 95}, {130, 205, 80}, 2);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 3, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    fill_checker(layer_pixels, k_layer_resolution, {95, 160, 210}, {70, 130, 190}, 2);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 4, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    fill_checker(layer_pixels, k_layer_resolution, {165, 195, 215}, {145, 175, 200}, 2);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 5, k_layer_resolution, k_layer_resolution, 1, GL_RGBA, GL_UNSIGNED_BYTE, layer_pixels.data());

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    pack.albedo_array = texture;
    pack.layer_count = k_layer_count;
    pack.layer_resolution = k_layer_resolution;
    return pack;
}

void destroy_material_pack(MaterialPack& pack) {
    if (pack.albedo_array != 0) {
        glDeleteTextures(1, &pack.albedo_array);
        pack.albedo_array = 0;
    }

    pack.layer_count = 0;
    pack.layer_resolution = 0;
}

} // namespace sandbox::voxel::render
