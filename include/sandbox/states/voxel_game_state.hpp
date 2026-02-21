#pragma once

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
    static constexpr float fixed_step_seconds_ = 1.0f / 60.0f;
};

} // namespace sandbox::states
