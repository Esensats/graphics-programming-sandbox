#pragma once

#include <glm/glm.hpp>

#include "sandbox/state.hpp"
#include "sandbox/voxel/render/material_pack.hpp"
#include "sandbox/voxel/runtime.hpp"

namespace sandbox::states {

class VoxelGameState final : public State {
  public:
    void on_enter(AppContext& context) override;
    void on_exit(AppContext& context) override;
    StateTransition update(AppContext& context, float delta_seconds) override;

  private:
    voxel::Runtime runtime_{};
    voxel::render::MaterialPack material_pack_{};
    unsigned int program_ = 0;
    float accumulator_seconds_ = 0.0f;
    float elapsed_seconds_ = 0.0f;
    glm::vec3 camera_position_{0.0f, 38.0f, 110.0f};
    float camera_yaw_degrees_ = -90.0f;
    float camera_pitch_degrees_ = -14.0f;
    double last_cursor_x_ = 0.0;
    double last_cursor_y_ = 0.0;
    bool look_active_last_frame_ = false;
    static constexpr float fixed_step_seconds_ = 1.0f / 60.0f;
};

} // namespace sandbox::states
