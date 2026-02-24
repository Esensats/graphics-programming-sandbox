#pragma once

#include <cstddef>

#include <glm/glm.hpp>

#include "sandbox/voxel/meshing/controller.hpp"
#include "sandbox/voxel/render/material_pack.hpp"

namespace sandbox::voxel::render {

struct RenderFrameInput {
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    glm::vec3 camera_world{};
    glm::mat4 view_projection{1.0f};
    meshing::VisibleDrawLists draw_lists{};
};

struct RenderFrameStats {
    std::size_t opaque_draw_count = 0;
    std::size_t cutout_draw_count = 0;
    std::size_t translucent_draw_count = 0;

    [[nodiscard]] std::size_t total_draw_count() const {
        return opaque_draw_count + cutout_draw_count + translucent_draw_count;
    }
};

class RenderSystem {
  public:
    void initialize(const char* resource_pack_directory);
    void shutdown();

    void render_frame(const RenderFrameInput& input) const;

    [[nodiscard]] bool initialized() const;
    [[nodiscard]] RenderFrameStats frame_stats(const meshing::VisibleDrawLists& draws) const;

  private:
    static void draw_commands(unsigned int program,
                              const glm::mat4& view_projection,
                              const std::vector<meshing::DrawCommand>& commands,
                              float alpha);

    unsigned int program_ = 0;
    MaterialPack material_pack_{};
};

} // namespace sandbox::voxel::render
