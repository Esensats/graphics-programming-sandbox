#pragma once

#include <functional>
#include <memory>
#include <string>

namespace sandbox {

struct AppContext;
struct StateTransition;

class State {
  public:
    virtual ~State() = default;

    virtual void on_enter(AppContext& context) = 0;
    virtual void on_exit(AppContext& context) = 0;
    virtual StateTransition update(AppContext& context, float delta_seconds) = 0;
};

struct StateEntry {
    std::string display_name;
    std::function<std::unique_ptr<State>()> factory;
};

struct StateTransition {
    bool quit_requested = false;
    std::function<std::unique_ptr<State>()> next_factory;

    static StateTransition none() { return {}; }

    static StateTransition quit() {
        StateTransition t;
        t.quit_requested = true;
        return t;
    }

    template<std::derived_from<State> T>
    static StateTransition to() {
        StateTransition t;
        t.next_factory = [] { return std::make_unique<T>(); };
        return t;
    }

    // For use when the factory already comes from a StateEntry (e.g. selector menu buttons)
    static StateTransition to_factory(std::function<std::unique_ptr<State>()> factory) {
        StateTransition t;
        t.next_factory = std::move(factory);
        return t;
    }
};

} // namespace sandbox
