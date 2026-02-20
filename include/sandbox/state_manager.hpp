#pragma once

#include <memory>

#include "sandbox/state.hpp"

namespace sandbox {

struct AppContext;

class StateManager {
  public:
    StateManager() = default;

    void initialize(AppContext& context, AppStateId initial_state);
    void shutdown(AppContext& context);
    bool update(AppContext& context, float delta_seconds);

  private:
    void switch_to(AppContext& context, AppStateId next_state);
    std::unique_ptr<State> make_state(AppStateId state_id);

    std::unique_ptr<State> active_state_;
    AppStateId active_state_id_ = AppStateId::selector_menu;
    bool initialized_ = false;
};

} // namespace sandbox
