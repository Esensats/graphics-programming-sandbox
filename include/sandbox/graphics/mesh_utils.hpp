#pragma once

#include <span>

namespace sandbox::graphics {

struct VertexAttribute {
    unsigned int index = 0;
    int component_count = 0;
    int stride_floats = 0;
    int offset_floats = 0;
};

struct IndexedMeshHandles {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    unsigned int ebo = 0;
    int index_count = 0;
};

bool create_indexed_mesh(std::span<const float> vertices,
                         std::span<const unsigned int> indices,
                         std::span<const VertexAttribute> attributes,
                         IndexedMeshHandles& mesh);

void destroy_indexed_mesh(IndexedMeshHandles& mesh);

} // namespace sandbox::graphics
