#pragma once

struct GLFWwindow;

namespace sandbox {

struct AppContext {
    GLFWwindow* window = nullptr;
    int framebuffer_width = 0;
    int framebuffer_height = 0;
    double time_seconds = 0.0;
    bool return_to_selector_requested = false;
};

} // namespace sandbox
