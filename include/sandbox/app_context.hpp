#pragma once

#include <optional>

#include "sandbox/state.hpp"
#include "sandbox/state_registry.hpp"

struct GLFWwindow;

namespace sandbox {

struct AppContext {
    GLFWwindow* window = nullptr;
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    double time_seconds = 0.0;

    // Set by the host (e.g. main loop) to request an immediate state switch
    // before the active state's update runs. Consumed and cleared by StateManager.
    std::optional<StateTransition> pending_transition;

    // Registry of states shown in the selector menu. Populated once in main()
    // before the loop starts. Read by SelectorMenuState to populate its buttons.
    StateRegistry state_registry;
};

} // namespace sandbox
