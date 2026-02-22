#pragma once

#include <glm/glm.hpp>

#include "sandbox/voxel/world/chunk_coords.hpp"

struct GLFWwindow;

namespace sandbox::voxel::camera {

class FlyCamera {
  public:
    void reset_default_pose();

    void set_position(const glm::vec3& position);
    [[nodiscard]] const glm::vec3& position() const;

    [[nodiscard]] float yaw_degrees() const;
    [[nodiscard]] float yaw_degrees_wrapped() const;
    [[nodiscard]] float pitch_degrees() const;
    void apply_look_delta(float delta_yaw_degrees, float delta_pitch_degrees);

    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 up() const;

    [[nodiscard]] glm::mat4 view_matrix() const;
    [[nodiscard]] glm::mat4 projection_matrix(int framebuffer_width, int framebuffer_height) const;

    [[nodiscard]] world::WorldVoxelCoord world_voxel_coord() const;
    [[nodiscard]] world::ChunkKey chunk_key() const;
    [[nodiscard]] world::LocalVoxelCoord local_coord() const;

    void move_forward(float amount);
    void move_right(float amount);
    void move_up(float amount);

  private:
    glm::vec3 position_{0.0f, 38.0f, 110.0f};
    float yaw_degrees_ = -90.0f;
    float pitch_degrees_ = -14.0f;
};

class FlyCameraController {
  public:
    void reset_cursor_tracking(GLFWwindow* window);
    void update_from_window(GLFWwindow* window, float delta_seconds, FlyCamera& camera, bool controls_enabled);

  private:
    double last_cursor_x_ = 0.0;
    double last_cursor_y_ = 0.0;
    bool look_active_last_frame_ = false;
};

} // namespace sandbox::voxel::camera
