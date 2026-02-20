#include "sandbox/states/voxel_game_state.hpp"

#include <glad/gl.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {

void VoxelGameState::on_enter(AppContext& context) {
    (void)context;
    runtime_.initialize();
    accumulator_seconds_ = 0.0f;
    LOG_INFO("Entered voxel game state (scaffold)");
}

void VoxelGameState::on_exit(AppContext& context) {
    (void)context;
    runtime_.shutdown();
    accumulator_seconds_ = 0.0f;
    LOG_INFO("Exited voxel game state (scaffold)");
}

StateTransition VoxelGameState::update(AppContext& context, float delta_seconds) {
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

    return StateTransition::none();
}

} // namespace sandbox::states
