#pragma once

#include <memory>

#include "sandbox/state.hpp"

namespace sandbox {

struct AppContext;

class StateManager {
  public:
    StateManager() = default;

    template<std::derived_from<State> InitialState>
    void initialize(AppContext& context) {
        initialize_impl(context, [] { return std::make_unique<InitialState>(); });
    }

    void shutdown(AppContext& context);
    bool update(AppContext& context, float delta_seconds);

  private:
    void initialize_impl(AppContext& context, std::function<std::unique_ptr<State>()> factory);
    void switch_to(AppContext& context, std::function<std::unique_ptr<State>()> factory);

    std::unique_ptr<State> active_state_;
    bool initialized_ = false;
};

} // namespace sandbox
