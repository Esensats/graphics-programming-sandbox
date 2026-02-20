#pragma once

namespace sandbox {

struct AppContext;

enum class AppStateId {
    selector_menu,
    fragment_playground,
    hello_cube,
    voxel_game,
};

struct StateTransition {
    bool quit_requested = false;
    bool has_next_state = false;
    AppStateId next_state = AppStateId::selector_menu;

    static StateTransition none() {
        return {};
    }

    static StateTransition quit() {
        StateTransition transition{};
        transition.quit_requested = true;
        return transition;
    }

    static StateTransition to(AppStateId id) {
        StateTransition transition{};
        transition.has_next_state = true;
        transition.next_state = id;
        return transition;
    }
};

class State {
  public:
    virtual ~State() = default;

    virtual void on_enter(AppContext& context) = 0;
    virtual void on_exit(AppContext& context) = 0;
    virtual StateTransition update(AppContext& context, float delta_seconds) = 0;
};

} // namespace sandbox
