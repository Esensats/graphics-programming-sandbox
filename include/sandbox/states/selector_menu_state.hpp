#pragma once

#include "sandbox/state.hpp"

namespace sandbox::states {

class SelectorMenuState final : public State {
  public:
    void on_enter(AppContext& context) override;
    void on_exit(AppContext& context) override;
    StateTransition update(AppContext& context, float delta_seconds) override;

  private:
    bool initialized_ = false;
};

} // namespace sandbox::states
