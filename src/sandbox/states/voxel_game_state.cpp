#include "sandbox/states/voxel_game_state.hpp"

#include <algorithm>
#include <cmath>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/app_context.hpp"
#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

#ifndef SANDBOX_RESOURCE_PACK_DIR
#define SANDBOX_RESOURCE_PACK_DIR "resource_packs"
#endif

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
    runtime_.initialize();
    material_pack_ = voxel::render::create_material_pack_from_directory(SANDBOX_RESOURCE_PACK_DIR "/default");

    program_ = graphics::create_program_from_files("voxel_chunk.vert", "voxel_chunk.frag");
    if (program_ == 0) {
        LOG_ERROR("Failed to create voxel program");
    } else {
        glUseProgram(program_);
        const int albedo_loc = glGetUniformLocation(program_, "u_albedo_array");
        glUniform1i(albedo_loc, 0);
    }

    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    camera_position_ = glm::vec3(0.0f, 38.0f, 110.0f);
    camera_yaw_degrees_ = -90.0f;
    camera_pitch_degrees_ = -14.0f;
    glfwGetCursorPos(context.window, &last_cursor_x_, &last_cursor_y_);
    look_active_last_frame_ = false;

    glEnable(GL_DEPTH_TEST);
    LOG_INFO("Entered voxel game state");
}

void VoxelGameState::on_exit(AppContext& context) {
    glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    look_active_last_frame_ = false;

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    voxel::render::destroy_material_pack(material_pack_);

    runtime_.shutdown();
    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    glDisable(GL_BLEND);
    LOG_INFO("Exited voxel game state");
}

StateTransition VoxelGameState::update(AppContext& context, float delta_seconds) {
    elapsed_seconds_ += delta_seconds;

    const bool look_active = glfwGetMouseButton(context.window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    if (look_active) {
        glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(context.window, &cursor_x, &cursor_y);
    if (look_active) {
        if (!look_active_last_frame_) {
            last_cursor_x_ = cursor_x;
            last_cursor_y_ = cursor_y;
        }

        const float dx = static_cast<float>(cursor_x - last_cursor_x_);
        const float dy = static_cast<float>(cursor_y - last_cursor_y_);
        const float look_sensitivity = 0.10f;

        camera_yaw_degrees_ += dx * look_sensitivity;
        camera_pitch_degrees_ -= dy * look_sensitivity;
        camera_pitch_degrees_ = std::clamp(camera_pitch_degrees_, -89.0f, 89.0f);
    }
    last_cursor_x_ = cursor_x;
    last_cursor_y_ = cursor_y;
    look_active_last_frame_ = look_active;

    const float yaw_rad = glm::radians(camera_yaw_degrees_);
    const float pitch_rad = glm::radians(camera_pitch_degrees_);
    const glm::vec3 camera_forward = glm::normalize(glm::vec3(
        std::cos(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        std::sin(yaw_rad) * std::cos(pitch_rad)));
    const glm::vec3 world_up = glm::vec3(0.0f, 1.0f, 0.0f);
    const glm::vec3 camera_right = glm::normalize(glm::cross(camera_forward, world_up));

    float move_speed = 24.0f;
    const bool boost = glfwGetKey(context.window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        || glfwGetKey(context.window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (boost) {
        move_speed *= 3.0f;
    }
    const float move_step = move_speed * delta_seconds;

    if (glfwGetKey(context.window, GLFW_KEY_W) == GLFW_PRESS) {
        camera_position_ += camera_forward * move_step;
    }
    if (glfwGetKey(context.window, GLFW_KEY_S) == GLFW_PRESS) {
        camera_position_ -= camera_forward * move_step;
    }
    if (glfwGetKey(context.window, GLFW_KEY_A) == GLFW_PRESS) {
        camera_position_ -= camera_right * move_step;
    }
    if (glfwGetKey(context.window, GLFW_KEY_D) == GLFW_PRESS) {
        camera_position_ += camera_right * move_step;
    }
    if (glfwGetKey(context.window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera_position_ += world_up * move_step;
    }
    if (glfwGetKey(context.window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
        || glfwGetKey(context.window, GLFW_KEY_C) == GLFW_PRESS) {
        camera_position_ -= world_up * move_step;
    }

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

    const glm::vec3 eye = camera_position_;
    const glm::vec3 target = camera_position_ + camera_forward;

    const glm::mat4 projection = glm::perspective(0.95f, aspect, 0.1f, 1200.0f);
    const glm::mat4 view = glm::lookAt(eye, target, world_up);
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
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, material_pack_.albedo_array);
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
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

    return StateTransition::none();
}

} // namespace sandbox::states
