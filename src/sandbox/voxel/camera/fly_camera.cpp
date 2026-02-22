#include "sandbox/voxel/camera/fly_camera.hpp"

#include <algorithm>
#include <cmath>

#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace sandbox::voxel::camera {

void FlyCamera::reset_default_pose() {
    position_ = glm::vec3(0.0f, 38.0f, 110.0f);
    yaw_degrees_ = -90.0f;
    pitch_degrees_ = -14.0f;
}

void FlyCamera::set_position(const glm::vec3& position) {
    position_ = position;
}

const glm::vec3& FlyCamera::position() const {
    return position_;
}

float FlyCamera::yaw_degrees() const {
    return yaw_degrees_;
}

float FlyCamera::yaw_degrees_wrapped() const {
    const float wrapped = std::fmod(yaw_degrees_, 360.0f);
    if (wrapped <= -180.0f) {
        return wrapped + 360.0f;
    }
    if (wrapped > 180.0f) {
        return wrapped - 360.0f;
    }
    return wrapped;
}

float FlyCamera::pitch_degrees() const {
    return pitch_degrees_;
}

void FlyCamera::apply_look_delta(float delta_yaw_degrees, float delta_pitch_degrees) {
    yaw_degrees_ += delta_yaw_degrees;
    pitch_degrees_ += delta_pitch_degrees;
    pitch_degrees_ = std::clamp(pitch_degrees_, -89.0f, 89.0f);
}

glm::vec3 FlyCamera::forward() const {
    const float yaw_rad = glm::radians(yaw_degrees_);
    const float pitch_rad = glm::radians(pitch_degrees_);

    return glm::normalize(glm::vec3(
        std::cos(yaw_rad) * std::cos(pitch_rad),
        std::sin(pitch_rad),
        std::sin(yaw_rad) * std::cos(pitch_rad)));
}

glm::vec3 FlyCamera::right() const {
    return glm::normalize(glm::cross(forward(), up()));
}

glm::vec3 FlyCamera::up() const {
    return glm::vec3(0.0f, 1.0f, 0.0f);
}

glm::mat4 FlyCamera::view_matrix() const {
    return glm::lookAt(position_, position_ + forward(), up());
}

glm::mat4 FlyCamera::projection_matrix(int framebuffer_width, int framebuffer_height) const {
    const int safe_height = framebuffer_height > 0 ? framebuffer_height : 1;
    const float aspect = static_cast<float>(framebuffer_width) / static_cast<float>(safe_height);
    return glm::perspective(0.95f, aspect, 0.1f, 1200.0f);
}

world::WorldVoxelCoord FlyCamera::world_voxel_coord() const {
    return world::WorldVoxelCoord{
        static_cast<int>(std::floor(position_.x)),
        static_cast<int>(std::floor(position_.y)),
        static_cast<int>(std::floor(position_.z)),
    };
}

world::ChunkKey FlyCamera::chunk_key() const {
    return world::world_to_chunk_key(world_voxel_coord());
}

world::LocalVoxelCoord FlyCamera::local_coord() const {
    return world::world_to_local_coord(world_voxel_coord());
}

void FlyCamera::move_forward(float amount) {
    position_ += forward() * amount;
}

void FlyCamera::move_right(float amount) {
    position_ += right() * amount;
}

void FlyCamera::move_up(float amount) {
    position_ += up() * amount;
}

void FlyCameraController::reset_cursor_tracking(GLFWwindow* window) {
    glfwGetCursorPos(window, &last_cursor_x_, &last_cursor_y_);
    look_active_last_frame_ = false;
}

void FlyCameraController::update_from_window(GLFWwindow* window,
                                             float delta_seconds,
                                             FlyCamera& camera,
                                             bool controls_enabled) {
    bool look_active = false;
    if (controls_enabled) {
        look_active = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    }

    if (look_active) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    } else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    double cursor_x = 0.0;
    double cursor_y = 0.0;
    glfwGetCursorPos(window, &cursor_x, &cursor_y);

    if (look_active) {
        if (!look_active_last_frame_) {
            last_cursor_x_ = cursor_x;
            last_cursor_y_ = cursor_y;
        }

        const float dx = static_cast<float>(cursor_x - last_cursor_x_);
        const float dy = static_cast<float>(cursor_y - last_cursor_y_);
        constexpr float look_sensitivity = 0.10f;
        camera.apply_look_delta(dx * look_sensitivity, -dy * look_sensitivity);
    }

    last_cursor_x_ = cursor_x;
    last_cursor_y_ = cursor_y;
    look_active_last_frame_ = look_active;

    if (!controls_enabled) {
        return;
    }

    float move_speed = 24.0f;
    const bool boost = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    if (boost) {
        move_speed *= 3.0f;
    }

    const float move_step = move_speed * delta_seconds;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        camera.move_forward(move_step);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        camera.move_forward(-move_step);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        camera.move_right(-move_step);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        camera.move_right(move_step);
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
        camera.move_up(move_step);
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS
        || glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) {
        camera.move_up(-move_step);
    }
}

} // namespace sandbox::voxel::camera
