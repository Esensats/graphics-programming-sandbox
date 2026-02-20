#include "sandbox/states/hello_cube_state.hpp"

#include <array>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/app_context.hpp"
#include "sandbox/graphics/mesh_utils.hpp"
#include "sandbox/graphics/primitives.hpp"
#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

constexpr std::array<graphics::VertexAttribute, 2> k_cube_vertex_attributes{{
    {0, 3, 6, 0},
    {1, 3, 6, 3},
}};

} // namespace

void HelloCubeState::on_enter(AppContext& context) {
    (void)context;

    const auto vertices = graphics::cube_vertices_pos_color();
    const auto indices = graphics::cube_indices();

    program_ = graphics::create_program_from_files("hello_cube.vert", "hello_cube.frag");
    const bool mesh_created =
        graphics::create_indexed_mesh(vertices, indices, k_cube_vertex_attributes, mesh_);
    if (!mesh_created) {
        LOG_ERROR("Failed to create hello cube mesh");
    }
    glEnable(GL_DEPTH_TEST);

    elapsed_seconds_ = 0.0f;
    LOG_INFO("Entered hello cube state");
}

void HelloCubeState::on_exit(AppContext& context) {
    (void)context;

    graphics::destroy_indexed_mesh(mesh_);

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    glDisable(GL_DEPTH_TEST);
    LOG_INFO("Exited hello cube state");
}

StateTransition HelloCubeState::update(AppContext& context, float delta_seconds) {
    elapsed_seconds_ += delta_seconds;

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.05f, 0.06f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (program_ == 0 || mesh_.vao == 0 || mesh_.index_count <= 0) {
        return StateTransition::none();
    }

    const float aspect = static_cast<float>(context.framebuffer_width) /
                         static_cast<float>(context.framebuffer_height > 0 ? context.framebuffer_height : 1);

    const glm::mat4 projection = glm::perspective(0.95f, aspect, 0.1f, 100.0f);
    const glm::mat4 view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -2.5f));
    const glm::mat4 model = glm::rotate(glm::mat4(1.0f), elapsed_seconds_, glm::vec3(0.0f, 1.0f, 0.0f)) *
                            glm::rotate(glm::mat4(1.0f), elapsed_seconds_ * 0.7f, glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::mat4 mvp = projection * view * model;

    glUseProgram(program_);
    const int mvp_loc = glGetUniformLocation(program_, "u_mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));

    glBindVertexArray(mesh_.vao);
    glDrawElements(GL_TRIANGLES, mesh_.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return StateTransition::none();
}

} // namespace sandbox::states
