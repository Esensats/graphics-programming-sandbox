#pragma once

#include <span>

namespace sandbox::graphics {

std::span<const float> cube_vertices_pos_color();
std::span<const unsigned int> cube_indices();

} // namespace sandbox::graphics
