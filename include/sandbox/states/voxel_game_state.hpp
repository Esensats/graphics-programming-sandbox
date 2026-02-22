#pragma once

#include "sandbox/state.hpp"
#include "sandbox/voxel/ui/game_overlay.hpp"
#include "sandbox/voxel/camera/fly_camera.hpp"
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
    voxel::ui::GameOverlay overlay_{};
    voxel::camera::FlyCamera camera_{};
    voxel::camera::FlyCameraController camera_controller_{};
    unsigned int program_ = 0;

    bool paused_ = false;
    bool show_debug_overlay_ = false;
    bool esc_pressed_last_frame_ = false;
    bool f3_pressed_last_frame_ = false;
    bool r_pressed_last_frame_ = false;
    bool a_pressed_last_frame_ = false;

    float accumulator_seconds_ = 0.0f;
    float elapsed_seconds_ = 0.0f;
    float frame_time_ms_ = 0.0f;
    float fps_ = 0.0f;
    float tps_ = 0.0f;
    float tps_window_accumulator_seconds_ = 0.0f;
    std::size_t fixed_steps_in_tps_window_ = 0;
    static constexpr float fixed_step_seconds_ = 1.0f / 60.0f;
};

} // namespace sandbox::states
