#include "sandbox/states/voxel_game_state.hpp"

#include <array>
#include <cmath>
#include <span>

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "sandbox/app_context.hpp"
#include "sandbox/graphics/primitives.hpp"
#include "sandbox/graphics/shader_utils.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

constexpr std::array<graphics::VertexAttribute, 2> k_cube_vertex_attributes{{
    {0, 3, 6, 0},
    {1, 3, 6, 3},
}};

void draw_chunk_proxy_group(unsigned int program,
                            const graphics::IndexedMeshHandles& mesh,
                            const glm::mat4& view_projection,
                            std::span<const voxel::world::ChunkKey> chunk_keys,
                            float chunk_scale) {
    const int mvp_loc = glGetUniformLocation(program, "u_mvp");

    const float chunk_extent = static_cast<float>(voxel::world::kChunkExtent);
    for (const voxel::world::ChunkKey& key : chunk_keys) {
        const glm::vec3 chunk_origin = glm::vec3(
            static_cast<float>(key.x) * chunk_extent,
            static_cast<float>(key.y) * chunk_extent,
            static_cast<float>(key.z) * chunk_extent);
        const glm::vec3 center_offset = glm::vec3(chunk_extent * 0.5f);

        const glm::mat4 model = glm::translate(glm::mat4(1.0f), chunk_origin + center_offset)
            * glm::scale(glm::mat4(1.0f), glm::vec3(chunk_extent * chunk_scale));
        const glm::mat4 mvp = view_projection * model;
        glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, glm::value_ptr(mvp));
        glDrawElements(GL_TRIANGLES, mesh.index_count, GL_UNSIGNED_INT, nullptr);
    }
}

} // namespace

void VoxelGameState::on_enter(AppContext& context) {
    (void)context;
    runtime_.initialize();

    program_ = graphics::create_program_from_files("hello_cube.vert", "hello_cube.frag");
    const auto vertices = graphics::cube_vertices_pos_color();
    const auto indices = graphics::cube_indices();
    const bool mesh_created = graphics::create_indexed_mesh(vertices, indices, k_cube_vertex_attributes, chunk_proxy_mesh_);
    if (!mesh_created) {
        LOG_ERROR("Failed to create voxel chunk proxy mesh");
    }

    accumulator_seconds_ = 0.0f;
    elapsed_seconds_ = 0.0f;
    glEnable(GL_DEPTH_TEST);
    LOG_INFO("Entered voxel game state");
}

void VoxelGameState::on_exit(AppContext& context) {
    (void)context;
    graphics::destroy_indexed_mesh(chunk_proxy_mesh_);
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

    if (program_ == 0 || chunk_proxy_mesh_.vao == 0 || chunk_proxy_mesh_.index_count <= 0) {
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
    const voxel::meshing::RenderPassBuckets visible = runtime_.visible_render_pass_buckets(voxel::meshing::VisibilityQuery{
        .origin_world = camera_world,
        .enable_distance_cull = true,
        .max_chunk_distance = 8,
        .enable_frustum_cull = false,
    });

    glUseProgram(program_);
    glBindVertexArray(chunk_proxy_mesh_.vao);

    glDisable(GL_BLEND);
    draw_chunk_proxy_group(program_, chunk_proxy_mesh_, view_projection, visible.opaque_chunks, 0.98f);
    draw_chunk_proxy_group(program_, chunk_proxy_mesh_, view_projection, visible.cutout_chunks, 0.96f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    draw_chunk_proxy_group(program_, chunk_proxy_mesh_, view_projection, visible.translucent_chunks, 0.94f);

    glDisable(GL_BLEND);
    glBindVertexArray(0);

    return StateTransition::none();
}

} // namespace sandbox::states
