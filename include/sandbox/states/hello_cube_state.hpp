#pragma once

#include "sandbox/state.hpp"

namespace sandbox::states {

class HelloCubeState final : public State {
  public:
    void on_enter(AppContext& context) override;
    void on_exit(AppContext& context) override;
    StateTransition update(AppContext& context, float delta_seconds) override;

  private:
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
    unsigned int vbo_ = 0;
    unsigned int ebo_ = 0;
    float elapsed_seconds_ = 0.0f;
};

} // namespace sandbox::states
