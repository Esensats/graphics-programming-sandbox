#pragma once

#include <memory>
#include <string>
#include <vector>

#include "sandbox/state.hpp"

namespace sandbox {

// Registry of states shown in the selector menu.
// Register a state here to make it appear as a button. States accessed only
// via programmatic transitions (StateTransition::to<T>()) need not be registered.
class StateRegistry {
  public:
    template<std::derived_from<State> T>
    void register_state(std::string display_name) {
        entries_.push_back({ std::move(display_name), [] { return std::make_unique<T>(); } });
    }

    [[nodiscard]] const std::vector<StateEntry>& entries() const { return entries_; }

  private:
    std::vector<StateEntry> entries_;
};

} // namespace sandbox
