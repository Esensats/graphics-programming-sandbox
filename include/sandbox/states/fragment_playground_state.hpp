#pragma once

#include "sandbox/state.hpp"

namespace sandbox::states {

class FragmentPlaygroundState final : public State {
  public:
    void on_enter(AppContext& context) override;
    void on_exit(AppContext& context) override;
    StateTransition update(AppContext& context, float delta_seconds) override;

  private:
    unsigned int program_ = 0;
    unsigned int vao_ = 0;
};

} // namespace sandbox::states
