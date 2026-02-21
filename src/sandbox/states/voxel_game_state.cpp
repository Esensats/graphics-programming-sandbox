#include "sandbox/states/voxel_game_state.hpp"

#include <cmath>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/app_context.hpp"
#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

void draw_commands(unsigned int program,
                   const glm::mat4& view_projection,
                   const std::vector<voxel::meshing::DrawCommand>& commands,
                   float alpha) {
    const int mvp_loc = glGetUniformLocation(program, "u_mvp");
    const int alpha_loc = glGetUniformLocation(program, "u_alpha");
    glUniform1f(alpha_loc, alpha);

    for (const voxel::meshing::DrawCommand& command : commands) {
        if (command.vao == 0 || command.index_count <= 0) {
            continue;
        }

        const glm::mat4 mvp = view_projection;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glBindVertexArray(command.vao);
        glDrawElements(GL_TRIANGLES, command.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

} // namespace

void VoxelGameState::on_enter(AppContext& context) {
    (void)context;
    runtime_.initialize();

    program_ = graphics::create_program_from_files("voxel_chunk.vert", "voxel_chunk.frag");
    if (program_ == 0) {
        LOG_ERROR("Failed to create voxel program");
    }

    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    glEnable(GL_DEPTH_TEST);
    LOG_INFO("Entered voxel game state");
}

void VoxelGameState::on_exit(AppContext& context) {
    (void)context;
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    runtime_.shutdown();
    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    glDisable(GL_BLEND);
    LOG_INFO("Exited voxel game state");
}

StateTransition VoxelGameState::update(AppContext& context, float delta_seconds) {
    elapsed_seconds_ += delta_seconds;

    accumulator_seconds_ += delta_seconds;
    while (accumulator_seconds_ >= fixed_step_seconds_) {
        runtime_.update_fixed(fixed_step_seconds_);
        accumulator_seconds_ -= fixed_step_seconds_;
    }
    runtime_.update_frame(delta_seconds);

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.03f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (program_ == 0) {
        return StateTransition::none();
    }

    const int safe_height = context.framebuffer_height > 0 ? context.framebuffer_height : 1;
    const float aspect = static_cast<float>(context.framebuffer_width) / static_cast<float>(safe_height);

    const float radius = 220.0f;
    const glm::vec3 target = glm::vec3(0.0f, 24.0f, 0.0f);
    const glm::vec3 eye = target + glm::vec3(
        std::cos(elapsed_seconds_ * 0.2f) * radius,
        90.0f,
        std::sin(elapsed_seconds_ * 0.2f) * radius);

    const glm::mat4 projection = glm::perspective(0.95f, aspect, 0.1f, 1200.0f);
    const glm::mat4 view = glm::lookAt(eye, target, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::mat4 view_projection = projection * view;

    const voxel::world::WorldVoxelCoord camera_world{
        static_cast<int>(eye.x),
        static_cast<int>(eye.y),
        static_cast<int>(eye.z),
    };
    const voxel::meshing::VisibleDrawLists visible_draws = runtime_.visible_draw_lists(voxel::meshing::VisibilityQuery{
        .origin_world = camera_world,
        .enable_distance_cull = true,
        .max_chunk_distance = 8,
        .enable_frustum_cull = false,
    });

    glUseProgram(program_);
    const int camera_loc = glGetUniformLocation(program_, "u_camera_world");
    glUniform3f(camera_loc, eye.x, eye.y, eye.z);

    const int fog_color_loc = glGetUniformLocation(program_, "u_fog_color");
    glUniform3f(fog_color_loc, 0.03f, 0.05f, 0.08f);

    const int fog_near_loc = glGetUniformLocation(program_, "u_fog_near");
    const int fog_far_loc = glGetUniformLocation(program_, "u_fog_far");
    glUniform1f(fog_near_loc, 140.0f);
    glUniform1f(fog_far_loc, 700.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    draw_commands(program_, view_projection, visible_draws.opaque, 1.0f);
    draw_commands(program_, view_projection, visible_draws.cutout, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    draw_commands(program_, view_projection, visible_draws.translucent, 0.55f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    return StateTransition::none();
}

} // namespace sandbox::states
