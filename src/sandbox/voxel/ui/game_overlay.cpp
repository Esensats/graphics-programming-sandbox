#include "sandbox/voxel/ui/game_overlay.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "sandbox/app_context.hpp"

namespace sandbox::voxel::ui {

void GameOverlay::on_enter(AppContext& context) {
    if (initialized_) {
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

    ImGui_ImplGlfw_InitForOpenGL(context.window, true);
    ImGui_ImplOpenGL3_Init("#version 460");
    initialized_ = true;
}

void GameOverlay::on_exit() {
    if (!initialized_) {
        return;
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    initialized_ = false;
}

void GameOverlay::begin_frame() {
    if (!initialized_) {
        return;
    }

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void GameOverlay::draw_debug_overlay(const DebugOverlayData& data) {
    if (!initialized_) {
        return;
    }

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
        | ImGuiWindowFlags_AlwaysAutoResize
        | ImGuiWindowFlags_NoSavedSettings
        | ImGuiWindowFlags_NoFocusOnAppearing
        | ImGuiWindowFlags_NoNav;

    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_Always);
    ImGui::SetNextWindowBgAlpha(0.62f);

    if (ImGui::Begin("Voxel Debug Overlay", nullptr, flags)) {
        ImGui::Text("FPS: %.1f", data.fps);
        ImGui::Text("Frametime: %.3f ms", data.frame_time_ms);
        ImGui::Text("TPS: %.1f", data.tps);
        ImGui::Separator();
        ImGui::Text("Camera Pos: %.2f, %.2f, %.2f", data.camera_position.x, data.camera_position.y, data.camera_position.z);
        ImGui::Text("Camera Rot: yaw %.2f, pitch %.2f", data.camera_yaw_degrees, data.camera_pitch_degrees);
        ImGui::Text("Chunk Key: %d, %d, %d", data.camera_chunk.x, data.camera_chunk.y, data.camera_chunk.z);
        ImGui::Text("Local Coords: %d, %d, %d", data.camera_local.x, data.camera_local.y, data.camera_local.z);
        ImGui::Separator();
        ImGui::Text("Active Chunks: %zu", data.active_chunk_count);
        ImGui::Text("Rendered Chunks: %zu", data.visible_chunk_count);
        ImGui::Text("Generation Queued: %zu", data.generation_queued_count);
        ImGui::Text("Upload Queued: %zu", data.upload_queued_count);
    }
    ImGui::End();
}

PauseOverlayResult GameOverlay::draw_pause_menu(bool paused) const {
    PauseOverlayResult result{};
    if (!initialized_ || !paused) {
        return result;
    }

    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->GetCenter(), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(340.0f, 180.0f), ImGuiCond_Always);

    constexpr ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    if (ImGui::Begin("Paused", nullptr, flags)) {
        ImGui::TextUnformatted("Simulation paused.");
        ImGui::Spacing();
        if (ImGui::Button("Resume", ImVec2(-1.0f, 0.0f))) {
            result.resume_requested = true;
        }
        if (ImGui::Button("Exit to selector", ImVec2(-1.0f, 0.0f))) {
            result.exit_to_selector_requested = true;
        }
    }
    ImGui::End();

    return result;
}

void GameOverlay::end_frame() {
    if (!initialized_) {
        return;
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

} // namespace sandbox::states
