#include "sandbox/graphics/mesh_utils.hpp"

#include <cstddef>

#include <glad/gl.h>

namespace sandbox::graphics {

bool create_indexed_mesh(std::span<const float> vertices,
                         std::span<const unsigned int> indices,
                         std::span<const VertexAttribute> attributes,
                         IndexedMeshHandles& mesh) {
    if (vertices.empty() || indices.empty() || attributes.empty()) {
        return false;
    }

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    for (const VertexAttribute& attribute : attributes) {
        glVertexAttribPointer(attribute.index,
                              attribute.component_count,
                              GL_FLOAT,
                              GL_FALSE,
                              attribute.stride_floats * static_cast<int>(sizeof(float)),
                              reinterpret_cast<const void*>(attribute.offset_floats * sizeof(float)));
        glEnableVertexAttribArray(attribute.index);
    }

    glBindVertexArray(0);
    mesh.index_count = static_cast<int>(indices.size());
    return true;
}

void destroy_indexed_mesh(IndexedMeshHandles& mesh) {
    if (mesh.ebo != 0) {
        glDeleteBuffers(1, &mesh.ebo);
        mesh.ebo = 0;
    }

    if (mesh.vbo != 0) {
        glDeleteBuffers(1, &mesh.vbo);
        mesh.vbo = 0;
    }

    if (mesh.vao != 0) {
        glDeleteVertexArrays(1, &mesh.vao);
        mesh.vao = 0;
    }

    mesh.index_count = 0;
}

} // namespace sandbox::graphics
