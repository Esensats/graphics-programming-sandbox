#include "sandbox/voxel/render/render_system.hpp"

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::voxel::render {

void RenderSystem::initialize(const char* resource_pack_directory) {
    shutdown();

    material_pack_ = create_material_pack_from_directory(resource_pack_directory);

    program_ = graphics::create_program_from_files("voxel_chunk.vert", "voxel_chunk.frag");
    if (program_ == 0) {
        LOG_ERROR("Failed to create voxel render program");
        return;
    }

    glUseProgram(program_);
    const int albedo_loc = glGetUniformLocation(program_, "u_albedo_array");
    glUniform1i(albedo_loc, 0);
}

void RenderSystem::shutdown() {
    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    destroy_material_pack(material_pack_);
}

void RenderSystem::render_frame(const RenderFrameInput& input) const {
    if (program_ == 0) {
        return;
    }

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, input.framebuffer_width, input.framebuffer_height);
    glClearColor(0.03f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(program_);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, material_pack_.albedo_array);

    const int camera_loc = glGetUniformLocation(program_, "u_camera_world");
    glUniform3f(camera_loc, input.camera_world.x, input.camera_world.y, input.camera_world.z);

    const int fog_color_loc = glGetUniformLocation(program_, "u_fog_color");
    glUniform3f(fog_color_loc, 0.03f, 0.05f, 0.08f);

    const int fog_near_loc = glGetUniformLocation(program_, "u_fog_near");
    const int fog_far_loc = glGetUniformLocation(program_, "u_fog_far");
    glUniform1f(fog_near_loc, 140.0f);
    glUniform1f(fog_far_loc, 700.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    draw_commands(program_, input.view_projection, input.draw_lists.opaque, 1.0f);
    draw_commands(program_, input.view_projection, input.draw_lists.cutout, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    draw_commands(program_, input.view_projection, input.draw_lists.translucent, 1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

bool RenderSystem::initialized() const {
    return program_ != 0 && material_pack_.albedo_array != 0;
}

RenderFrameStats RenderSystem::frame_stats(const meshing::VisibleDrawLists& draws) const {
    return RenderFrameStats{
        .opaque_draw_count = draws.opaque.size(),
        .cutout_draw_count = draws.cutout.size(),
        .translucent_draw_count = draws.translucent.size(),
    };
}

void RenderSystem::draw_commands(unsigned int program,
                                 const glm::mat4& view_projection,
                                 const std::vector<meshing::DrawCommand>& commands,
                                 float alpha) {
    const int mvp_loc = glGetUniformLocation(program, "u_mvp");
    const int alpha_loc = glGetUniformLocation(program, "u_alpha");
    glUniform1f(alpha_loc, alpha);

    for (const meshing::DrawCommand& command : commands) {
        if (command.vao == 0 || command.index_count <= 0) {
            continue;
        }

        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(view_projection));
        glBindVertexArray(command.vao);
        glDrawElements(GL_TRIANGLES, command.index_count, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

} // namespace sandbox::voxel::render
