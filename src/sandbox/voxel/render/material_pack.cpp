#include "sandbox/voxel/render/material_pack.hpp"

#include <array>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <glad/gl.h>

#include "sandbox/assets/image_loader.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::voxel::render {
namespace {

constexpr int k_layer_resolution = 16;
constexpr int k_layer_count = 6;

struct LayerFallback {
    std::array<unsigned char, 3> a{};
    std::array<unsigned char, 3> b{};
    int checker_size = 4;
};

constexpr std::array<const char*, k_layer_count> k_layer_keys{{
    "air",
    "stone",
    "slab",
    "grass",
    "water",
    "glass",
}};

constexpr std::array<const char*, k_layer_count> k_default_files{{
    "air.png",
    "stone.png",
    "slab.png",
    "grass.png",
    "water.png",
    "glass.png",
}};

constexpr std::array<LayerFallback, k_layer_count> k_fallbacks{{
    LayerFallback{{0, 0, 0}, {0, 0, 0}, 4},
    LayerFallback{{110, 110, 115}, {95, 95, 100}, 4},
    LayerFallback{{135, 185, 95}, {120, 165, 85}, 4},
    LayerFallback{{165, 225, 95}, {130, 205, 80}, 2},
    LayerFallback{{95, 160, 210}, {70, 130, 190}, 2},
    LayerFallback{{165, 195, 215}, {145, 175, 200}, 2},
}};

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

[[nodiscard]] std::string trim(std::string value) {
    auto is_space = [](unsigned char ch) {
        return std::isspace(ch) != 0;
    };

    while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

[[nodiscard]] std::unordered_map<std::string, std::string> parse_manifest(const std::filesystem::path& path) {
    std::unordered_map<std::string, std::string> manifest;

    std::ifstream input(path);
    if (!input.is_open()) {
        return manifest;
    }

    std::string line;
    while (std::getline(input, line)) {
        line = trim(line);
        if (line.empty() || line.front() == '#') {
            continue;
        }

        const std::size_t equals = line.find('=');
        if (equals == std::string::npos) {
            continue;
        }

        std::string key = trim(line.substr(0, equals));
        std::string value = trim(line.substr(equals + 1));
        if (!key.empty() && !value.empty()) {
            manifest[key] = value;
        }
    }

    return manifest;
}

void upload_placeholder_layer(int layer_index,
                              int resolution,
                              const LayerFallback& fallback) {
    std::vector<unsigned char> layer_pixels;
    fill_checker(layer_pixels, resolution, fallback.a, fallback.b, fallback.checker_size);
    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,
                    0,
                    0,
                    layer_index,
                    resolution,
                    resolution,
                    1,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    layer_pixels.data());
}

void upload_image_layer(int layer_index,
                        int resolution,
                        const assets::ImageData& image) {
    if (image.width == resolution && image.height == resolution) {
        glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                        0,
                        0,
                        0,
                        layer_index,
                        resolution,
                        resolution,
                        1,
                        GL_RGBA,
                        GL_UNSIGNED_BYTE,
                        image.pixels.data());
        return;
    }

    std::vector<unsigned char> resized(static_cast<std::size_t>(resolution * resolution * 4), 255);
    for (int y = 0; y < resolution; ++y) {
        for (int x = 0; x < resolution; ++x) {
            const int src_x = (x * image.width) / resolution;
            const int src_y = (y * image.height) / resolution;

            const std::size_t src_i = static_cast<std::size_t>((src_y * image.width + src_x) * 4);
            const std::size_t dst_i = static_cast<std::size_t>((y * resolution + x) * 4);

            resized[dst_i + 0] = image.pixels[src_i + 0];
            resized[dst_i + 1] = image.pixels[src_i + 1];
            resized[dst_i + 2] = image.pixels[src_i + 2];
            resized[dst_i + 3] = image.pixels[src_i + 3];
        }
    }

    glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
                    0,
                    0,
                    0,
                    layer_index,
                    resolution,
                    resolution,
                    1,
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    resized.data());
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

MaterialPack create_material_pack_from_directory(const char* directory_path) {
    if (directory_path == nullptr || directory_path[0] == '\0') {
        return create_placeholder_material_pack();
    }

    const std::filesystem::path base(directory_path);
    const auto manifest = parse_manifest(base / "pack.txt");

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

    bool used_any_file = false;
    for (int layer = 0; layer < k_layer_count; ++layer) {
        std::string filename = k_default_files[static_cast<std::size_t>(layer)];
        const auto manifest_it = manifest.find(k_layer_keys[static_cast<std::size_t>(layer)]);
        if (manifest_it != manifest.end()) {
            filename = manifest_it->second;
        }

        const std::filesystem::path file_path = base / filename;
        if (std::filesystem::exists(file_path)) {
            const auto image = assets::load_image_rgba8(file_path.string());
            if (image.has_value() && image->width > 0 && image->height > 0 && image->channels == 4) {
                upload_image_layer(layer, k_layer_resolution, *image);
                used_any_file = true;
                continue;
            }
        }

        upload_placeholder_layer(layer,
                                 k_layer_resolution,
                                 k_fallbacks[static_cast<std::size_t>(layer)]);
    }

    glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    pack.albedo_array = texture;
    pack.layer_count = k_layer_count;
    pack.layer_resolution = k_layer_resolution;

    if (used_any_file) {
        LOG_INFO("Loaded resource pack textures from '{}'", base.string());
    } else {
        LOG_WARN("No valid textures found in '{}'; using placeholders", base.string());
    }

    return pack;
}

} // namespace sandbox::voxel::render
