#include "sandbox/state_manager.hpp"

#include <utility>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"
#include "sandbox/states/fragment_playground_state.hpp"
#include "sandbox/states/hello_cube_state.hpp"
#include "sandbox/states/selector_menu_state.hpp"
#include "sandbox/states/voxel_game_state.hpp"

namespace sandbox {

void StateManager::initialize(AppContext& context, AppStateId initial_state) {
    if (initialized_) {
        switch_to(context, initial_state);
        return;
    }

    initialized_ = true;
    switch_to(context, initial_state);
}

void StateManager::shutdown(AppContext& context) {
    if (active_state_ != nullptr) {
        active_state_->on_exit(context);
        active_state_.reset();
    }
    initialized_ = false;
}

bool StateManager::update(AppContext& context, float delta_seconds) {
    if (!initialized_ || active_state_ == nullptr) {
        return false;
    }

    if (context.return_to_selector_requested && active_state_id_ != AppStateId::selector_menu) {
        switch_to(context, AppStateId::selector_menu);
        context.return_to_selector_requested = false;
    }

    const StateTransition transition = active_state_->update(context, delta_seconds);
    if (transition.quit_requested) {
        active_state_->on_exit(context);
        active_state_.reset();
        initialized_ = false;
        return false;
    }

    if (transition.has_next_state && transition.next_state != active_state_id_) {
        switch_to(context, transition.next_state);
    }

    return true;
}

void StateManager::switch_to(AppContext& context, AppStateId next_state) {
    if (active_state_ != nullptr) {
        active_state_->on_exit(context);
        active_state_.reset();
    }

    active_state_id_ = next_state;
    active_state_ = make_state(next_state);
    if (active_state_ == nullptr) {
        LOG_ERROR("Failed to create requested state");
        initialized_ = false;
        return;
    }

    active_state_->on_enter(context);
}

std::unique_ptr<State> StateManager::make_state(AppStateId state_id) {
    switch (state_id) {
        case AppStateId::selector_menu:
            return std::make_unique<states::SelectorMenuState>();
        case AppStateId::fragment_playground:
            return std::make_unique<states::FragmentPlaygroundState>();
        case AppStateId::hello_cube:
            return std::make_unique<states::HelloCubeState>();
        case AppStateId::voxel_game:
            return std::make_unique<states::VoxelGameState>();
    }

    return nullptr;
}

} // namespace sandbox
