#include "sandbox/states/voxel_game_state.hpp"

#include <cstddef>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
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

bool consume_edge_press(bool key_pressed_now, bool& key_pressed_last_frame) {
    const bool edge_pressed = key_pressed_now && !key_pressed_last_frame;
    key_pressed_last_frame = key_pressed_now;
    return edge_pressed;
}

} // namespace

void VoxelGameState::on_enter(AppContext& context) {
    runtime_.initialize();
    overlay_.on_enter(context);
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
    paused_ = false;
    show_debug_overlay_ = false;
    esc_pressed_last_frame_ = false;
    f3_pressed_last_frame_ = false;
    r_pressed_last_frame_ = false;
    a_pressed_last_frame_ = false;
    frame_time_ms_ = 0.0f;
    fps_ = 0.0f;
    tps_ = 0.0f;
    tps_window_accumulator_seconds_ = 0.0f;
    fixed_steps_in_tps_window_ = 0;

    camera_.reset_default_pose();
    camera_controller_.reset_cursor_tracking(context.window);

    glEnable(GL_DEPTH_TEST);
    LOG_INFO("Entered voxel game state");
}

void VoxelGameState::on_exit(AppContext& context) {
    glfwSetInputMode(context.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    overlay_.on_exit();
    voxel::render::destroy_material_pack(material_pack_);

    runtime_.shutdown();
    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    glDisable(GL_BLEND);
    LOG_INFO("Exited voxel game state");
}

StateTransition VoxelGameState::update(AppContext& context, float delta_seconds) {
    elapsed_seconds_ += delta_seconds;
    frame_time_ms_ = delta_seconds * 1000.0f;
    if (delta_seconds > 0.0f) {
        fps_ = 1.0f / delta_seconds;
    }

    const bool f3_pressed = glfwGetKey(context.window, GLFW_KEY_F3) == GLFW_PRESS;
    const bool r_edge_pressed = consume_edge_press(glfwGetKey(context.window, GLFW_KEY_R) == GLFW_PRESS, r_pressed_last_frame_);
    const bool a_edge_pressed = consume_edge_press(glfwGetKey(context.window, GLFW_KEY_A) == GLFW_PRESS, a_pressed_last_frame_);

    if (f3_pressed && r_edge_pressed) {
        runtime_.debug_regenerate_loaded_chunks();
    }
    if (f3_pressed && a_edge_pressed) {
        runtime_.debug_mark_all_chunks_dirty_mesh();
    }

    if (consume_edge_press(f3_pressed, f3_pressed_last_frame_)) {
        show_debug_overlay_ = !show_debug_overlay_;
    }
    if (consume_edge_press(glfwGetKey(context.window, GLFW_KEY_ESCAPE) == GLFW_PRESS, esc_pressed_last_frame_)) {
        paused_ = !paused_;
        accumulator_seconds_ = 0.0f;
        tps_window_accumulator_seconds_ = 0.0f;
        fixed_steps_in_tps_window_ = 0;
        if (paused_) {
            tps_ = 0.0f;
        }
    }

    camera_controller_.update_from_window(context.window, delta_seconds, camera_, !paused_);

    std::size_t fixed_steps_this_frame = 0;
    if (!paused_) {
        accumulator_seconds_ += delta_seconds;
        while (accumulator_seconds_ >= fixed_step_seconds_) {
            runtime_.update_fixed(fixed_step_seconds_);
            accumulator_seconds_ -= fixed_step_seconds_;
            ++fixed_steps_this_frame;
        }
    }

    runtime_.update_frame(delta_seconds);

    if (!paused_) {
        fixed_steps_in_tps_window_ += fixed_steps_this_frame;
        tps_window_accumulator_seconds_ += delta_seconds;
        if (tps_window_accumulator_seconds_ >= 0.25f) {
            if (tps_window_accumulator_seconds_ > 0.0f) {
                tps_ = static_cast<float>(fixed_steps_in_tps_window_) / tps_window_accumulator_seconds_;
            } else {
                tps_ = 0.0f;
            }
            tps_window_accumulator_seconds_ = 0.0f;
            fixed_steps_in_tps_window_ = 0;
        }
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.03f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    const glm::vec3 eye = camera_.position();
    const glm::mat4 projection = camera_.projection_matrix(context.framebuffer_width, context.framebuffer_height);
    const glm::mat4 view = camera_.view_matrix();
    const glm::mat4 view_projection = projection * view;

    const voxel::world::WorldVoxelCoord camera_world = camera_.world_voxel_coord();
    const voxel::meshing::VisibleDrawLists visible_draws = runtime_.visible_draw_lists(voxel::meshing::VisibilityQuery{
        .origin_world = camera_world,
        .enable_distance_cull = true,
        .max_chunk_distance = 8,
        .enable_frustum_cull = false,
    });

    const std::size_t visible_chunk_count = visible_draws.opaque.size()
        + visible_draws.cutout.size()
        + visible_draws.translucent.size();

    if (program_ != 0) {
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
        draw_commands(program_, view_projection, visible_draws.translucent, 1.0f);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
    }

    overlay_.begin_frame();
    if (show_debug_overlay_) {
        const voxel::RuntimeDebugSnapshot runtime_snapshot = runtime_.debug_snapshot();
        overlay_.draw_debug_overlay(voxel::ui::DebugOverlayData{
            .fps = fps_,
            .frame_time_ms = frame_time_ms_,
            .tps = tps_,
            .camera_position = eye,
            .camera_yaw_degrees = camera_.yaw_degrees_wrapped(),
            .camera_pitch_degrees = camera_.pitch_degrees(),
            .camera_chunk = camera_.chunk_key(),
            .camera_local = camera_.local_coord(),
            .active_chunk_count = runtime_snapshot.active_chunk_count,
            .visible_chunk_count = visible_chunk_count,
            .generation_queued_count = runtime_snapshot.generation_queued_count,
            .upload_queued_count = runtime_snapshot.upload_queued_count,
        });
    }

    const voxel::ui::PauseOverlayResult pause_result = overlay_.draw_pause_menu(paused_);
    overlay_.end_frame();

    if (pause_result.resume_requested) {
        paused_ = false;
    }
    if (pause_result.exit_to_selector_requested) {
        return StateTransition::to(AppStateId::selector_menu);
    }

    return StateTransition::none();
}

} // namespace sandbox::states
