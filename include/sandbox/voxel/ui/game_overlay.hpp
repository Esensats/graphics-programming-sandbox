#pragma once

#include <cstddef>

#include <glm/glm.hpp>

#include "sandbox/voxel/world/chunk_coords.hpp"

namespace sandbox {
struct AppContext;
}

namespace sandbox::voxel::ui {

struct DebugOverlayData {
    float fps = 0.0f;
    float frame_time_ms = 0.0f;
    float tps = 0.0f;
    glm::vec3 camera_position{0.0f};
    float camera_yaw_degrees = 0.0f;
    float camera_pitch_degrees = 0.0f;
    voxel::world::ChunkKey camera_chunk{};
    voxel::world::LocalVoxelCoord camera_local{};
    std::size_t active_chunk_count = 0;
    std::size_t visible_chunk_count = 0;
    std::size_t generation_queued_count = 0;
    std::size_t upload_queued_count = 0;
};

struct PauseOverlayResult {
    bool resume_requested = false;
    bool exit_to_selector_requested = false;
};

class GameOverlay {
  public:
    void on_enter(AppContext& context);
    void on_exit();

    void begin_frame();
    void draw_debug_overlay(const DebugOverlayData& data);
    [[nodiscard]] PauseOverlayResult draw_pause_menu(bool paused) const;
    void end_frame();

  private:
    bool initialized_ = false;
};

} // namespace sandbox::states
