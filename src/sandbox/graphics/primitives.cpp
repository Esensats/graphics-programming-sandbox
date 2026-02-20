#include "sandbox/graphics/primitives.hpp"

#include <array>

namespace sandbox::graphics {

std::span<const float> cube_vertices_pos_color() {
    static constexpr std::array<float, 48> vertices = {
        -0.5f, -0.5f, -0.5f, 1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, -0.5f, 0.2f, 1.0f, 0.2f,
         0.5f,  0.5f, -0.5f, 0.2f, 0.2f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.2f,
        -0.5f, -0.5f,  0.5f, 1.0f, 0.2f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.2f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.3f, 0.3f, 0.3f,
    };
    return vertices;
}

std::span<const unsigned int> cube_indices() {
    static constexpr std::array<unsigned int, 36> indices = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        5, 4, 7, 7, 6, 5,
        4, 0, 3, 3, 7, 4,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4,
    };
    return indices;
}

} // namespace sandbox::graphics
