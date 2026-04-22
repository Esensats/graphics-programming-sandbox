#include "sandbox/state_manager.hpp"

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"

namespace sandbox {

void StateManager::initialize_impl(AppContext& context, std::function<std::unique_ptr<State>()> factory) {
    if (initialized_) {
        switch_to(context, std::move(factory));
        return;
    }

    initialized_ = true;
    switch_to(context, std::move(factory));
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

    if (context.pending_transition.has_value()) {
        StateTransition pending = std::move(*context.pending_transition);
        context.pending_transition.reset();
        if (pending.quit_requested) {
            active_state_->on_exit(context);
            active_state_.reset();
            initialized_ = false;
            return false;
        }
        if (pending.next_factory) {
            switch_to(context, std::move(pending.next_factory));
        }
    }

    const StateTransition transition = active_state_->update(context, delta_seconds);

    if (transition.quit_requested) {
        active_state_->on_exit(context);
        active_state_.reset();
        initialized_ = false;
        return false;
    }

    if (transition.next_factory) {
        switch_to(context, transition.next_factory);
    }

    return true;
}

void StateManager::switch_to(AppContext& context, std::function<std::unique_ptr<State>()> factory) {
    if (active_state_ != nullptr) {
        active_state_->on_exit(context);
        active_state_.reset();
    }

    active_state_ = factory();
    if (active_state_ == nullptr) {
        LOG_ERROR("State factory returned nullptr");
        initialized_ = false;
        return;
    }

    active_state_->on_enter(context);
}

} // namespace sandbox
