#pragma once

#include <optional>
#include <string>
#include <vector>

namespace sandbox::assets {

struct ImageData {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<unsigned char> pixels;
};

std::optional<ImageData> load_image_rgba8(const std::string& path);

} // namespace sandbox::assets
