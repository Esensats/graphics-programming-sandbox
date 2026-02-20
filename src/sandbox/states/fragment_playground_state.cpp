#include "sandbox/states/fragment_playground_state.hpp"

#include <glad/gl.h>

#include "sandbox/app_context.hpp"
#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {

void FragmentPlaygroundState::on_enter(AppContext& context) {
    (void)context;

    program_ = graphics::create_program_from_files("fragment_playground.vert", "fragment_playground.frag");
    glGenVertexArrays(1, &vao_);
    LOG_INFO("Entered fragment shader playground state");
}

void FragmentPlaygroundState::on_exit(AppContext& context) {
    (void)context;

    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    LOG_INFO("Exited fragment shader playground state");
}

StateTransition FragmentPlaygroundState::update(AppContext& context, float delta_seconds) {
    (void)delta_seconds;

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (program_ == 0 || vao_ == 0) {
        return StateTransition::none();
    }

    glUseProgram(program_);

    const int resolution_loc = glGetUniformLocation(program_, "u_resolution");
    glUniform2f(resolution_loc, static_cast<float>(context.framebuffer_width),
                static_cast<float>(context.framebuffer_height));

    const int time_loc = glGetUniformLocation(program_, "u_time");
    glUniform1f(time_loc, static_cast<float>(context.time_seconds));

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    return StateTransition::none();
}

} // namespace sandbox::states
