#include "sandbox/states/selector_menu_state.hpp"

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {

void SelectorMenuState::on_enter(AppContext& context) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(context.window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    initialized_ = true;

    LOG_INFO("Entered selector menu state");
}

void SelectorMenuState::on_exit(AppContext& context) {
    (void)context;
    if (!initialized_) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;

    LOG_INFO("Exited selector menu state");
}

StateTransition SelectorMenuState::update(AppContext& context, float delta_seconds) {
    (void)delta_seconds;

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(420.0f, 220.0f), ImGuiCond_Always);

    StateTransition transition = StateTransition::none();
    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Experiment Selector", nullptr, flags)) {
        ImGui::TextUnformatted("Choose a state to launch:");
        ImGui::Spacing();

        if (ImGui::Button("Fragment Shader Playground", ImVec2(-1.0f, 0.0f))) {
            transition = StateTransition::to(AppStateId::fragment_playground);
        }
        if (ImGui::Button("Hello Cube", ImVec2(-1.0f, 0.0f))) {
            transition = StateTransition::to(AppStateId::hello_cube);
        }
        if (ImGui::Button("Voxel Game (Scaffold)", ImVec2(-1.0f, 0.0f))) {
            transition = StateTransition::to(AppStateId::voxel_game);
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextUnformatted("Press Shift+Esc from any state to return here.");
        if (ImGui::Button("Quit", ImVec2(-1.0f, 0.0f))) {
            transition = StateTransition::quit();
        }
    }
    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    return transition;
}

} // namespace sandbox::states
