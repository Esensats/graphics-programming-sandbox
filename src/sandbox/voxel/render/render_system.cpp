#include "sandbox/voxel/render/render_system.hpp"

#include <algorithm>

#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::voxel::render {

void RenderSystem::initialize(const char* resource_pack_directory) {
    shutdown();

    material_pack_ = create_material_pack_from_directory(resource_pack_directory);

    opaque_program_ = graphics::create_program_from_files("voxel_chunk_opaque.vert", "voxel_chunk_opaque.frag");
    cutout_program_ = graphics::create_program_from_files("voxel_chunk_cutout.vert", "voxel_chunk_cutout.frag");
    translucent_program_ = graphics::create_program_from_files("voxel_chunk_translucent.vert", "voxel_chunk_translucent.frag");

    if (opaque_program_ == 0 || cutout_program_ == 0 || translucent_program_ == 0) {
        LOG_ERROR("Failed to create voxel render programs");
        shutdown();
        return;
    }
}

void RenderSystem::shutdown() {
    if (opaque_program_ != 0) {
        glDeleteProgram(opaque_program_);
        opaque_program_ = 0;
    }

    if (cutout_program_ != 0) {
        glDeleteProgram(cutout_program_);
        cutout_program_ = 0;
    }

    if (translucent_program_ != 0) {
        glDeleteProgram(translucent_program_);
        translucent_program_ = 0;
    }

    destroy_material_pack(material_pack_);
}

void RenderSystem::render_frame(const RenderFrameInput& input) const {
    if (opaque_program_ == 0 || cutout_program_ == 0 || translucent_program_ == 0) {
        return;
    }

    if (material_pack_.albedo_array == 0) {
        return;
    }

    std::vector<meshing::DrawCommand> opaque_commands = input.draw_lists.opaque;
    std::vector<meshing::DrawCommand> translucent_commands = input.draw_lists.translucent;

    sort_opaque_front_to_back(opaque_commands, input.camera_world);
    sort_translucent_back_to_front(translucent_commands, input.camera_world);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    glViewport(0, 0, input.framebuffer_width, input.framebuffer_height);
    glClearColor(0.03f, 0.05f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, material_pack_.albedo_array);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    apply_common_uniforms(opaque_program_, input, material_pack_.albedo_array);
    draw_commands(opaque_program_, input.view_projection, opaque_commands, 1.0f);

    apply_common_uniforms(cutout_program_, input, material_pack_.albedo_array);
    draw_commands(cutout_program_, input.view_projection, input.draw_lists.cutout, 1.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    apply_common_uniforms(translucent_program_, input, material_pack_.albedo_array);
    draw_commands(translucent_program_, input.view_projection, translucent_commands, 1.0f);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}

bool RenderSystem::initialized() const {
    return opaque_program_ != 0
        && cutout_program_ != 0
        && translucent_program_ != 0
        && material_pack_.albedo_array != 0;
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

void RenderSystem::apply_common_uniforms(unsigned int program,
                                         const RenderFrameInput& input,
                                         unsigned int albedo_array) {
    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D_ARRAY, albedo_array);

    const int albedo_loc = glGetUniformLocation(program, "u_albedo_array");
    glUniform1i(albedo_loc, 0);

    const int camera_loc = glGetUniformLocation(program, "u_camera_world");
    glUniform3f(camera_loc, input.camera_world.x, input.camera_world.y, input.camera_world.z);

    const int fog_color_loc = glGetUniformLocation(program, "u_fog_color");
    glUniform3f(fog_color_loc, 0.03f, 0.05f, 0.08f);

    const int fog_near_loc = glGetUniformLocation(program, "u_fog_near");
    const int fog_far_loc = glGetUniformLocation(program, "u_fog_far");
    glUniform1f(fog_near_loc, 140.0f);
    glUniform1f(fog_far_loc, 700.0f);
}

void RenderSystem::sort_opaque_front_to_back(std::vector<meshing::DrawCommand>& commands,
                                             const glm::vec3& camera_world) {
    auto distance_sq = [&camera_world](const meshing::DrawCommand& command) {
        const float dx = command.sort_center_x - camera_world.x;
        const float dy = command.sort_center_y - camera_world.y;
        const float dz = command.sort_center_z - camera_world.z;
        return dx * dx + dy * dy + dz * dz;
    };

    std::sort(commands.begin(), commands.end(), [&distance_sq](const meshing::DrawCommand& lhs, const meshing::DrawCommand& rhs) {
        return distance_sq(lhs) < distance_sq(rhs);
    });
}

void RenderSystem::sort_translucent_back_to_front(std::vector<meshing::DrawCommand>& commands,
                                                  const glm::vec3& camera_world) {
    auto distance_sq = [&camera_world](const meshing::DrawCommand& command) {
        const float dx = command.sort_center_x - camera_world.x;
        const float dy = command.sort_center_y - camera_world.y;
        const float dz = command.sort_center_z - camera_world.z;
        return dx * dx + dy * dy + dz * dz;
    };

    std::sort(commands.begin(), commands.end(), [&distance_sq](const meshing::DrawCommand& lhs, const meshing::DrawCommand& rhs) {
        return distance_sq(lhs) > distance_sq(rhs);
    });
}

} // namespace sandbox::voxel::render
