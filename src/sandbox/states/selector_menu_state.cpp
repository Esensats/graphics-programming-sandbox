#include "sandbox/states/selector_menu_state.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"
#include "sandbox/states/state_imgui_utils.hpp"

namespace sandbox::states {

void SelectorMenuState::on_enter(AppContext& context) {
    imgui_utils::initialize_for_context(context);
    initialized_ = true;

    LOG_INFO("Entered selector menu state");
}

void SelectorMenuState::on_exit(AppContext& context) {
    (void)context;
    if (!initialized_) {
        return;
    }

    imgui_utils::shutdown();
    initialized_ = false;

    LOG_INFO("Exited selector menu state");
}

StateTransition SelectorMenuState::update(AppContext& context, float delta_seconds) {
    (void)delta_seconds;

    imgui_utils::clear_framebuffer(0.08f, 0.08f, 0.12f, context);
    imgui_utils::begin_frame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420.0f, 220.0f), ImGuiCond_Always);

    StateTransition transition = StateTransition::none();
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Experiment Selector", nullptr, flags)) {
        ImGui::TextUnformatted("Choose a state to launch:");
        ImGui::Spacing();

        for (const auto& entry : context.state_registry.entries()) {
            if (ImGui::Button(entry.display_name.c_str(), ImVec2(-1.0f, 0.0f))) {
                transition = StateTransition::to_factory(entry.factory);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Press Shift+Esc from any state to return here.");
        if (ImGui::Button("Quit", ImVec2(-1.0f, 0.0f))) {
            transition = StateTransition::quit();
        }
    }
    ImGui::End();

    imgui_utils::end_frame();
    return transition;
}

} // namespace sandbox::states
