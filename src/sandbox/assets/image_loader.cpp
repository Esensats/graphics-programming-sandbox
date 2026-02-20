#include "sandbox/assets/image_loader.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "sandbox/logging.hpp"

namespace sandbox::assets {

std::optional<ImageData> load_image_rgba8(const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;

    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        LOG_ERROR("Failed to load image '{}': {}", path, stbi_failure_reason());
        return std::nullopt;
    }

    ImageData image{};
    image.width = width;
    image.height = height;
    image.channels = 4;

    const std::size_t byte_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    image.pixels.assign(pixels, pixels + byte_count);

    stbi_image_free(pixels);
    return image;
}

} // namespace sandbox::assets
