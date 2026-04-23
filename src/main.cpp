#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"
#include "sandbox/state_manager.hpp"
#include "sandbox/states/fragment_playground_state.hpp"
#include "sandbox/states/hello_cube_state.hpp"
#include "sandbox/states/selector_menu_state.hpp"
#include "sandbox/states/voxel_game_state.hpp"

namespace {
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    (void)window;
    glViewport(0, 0, width, height);
}
}

int main() {
    sandbox::logging::Config logging_config{};
#if defined(NDEBUG)
    logging_config.level = spdlog::level::info;
#else
    logging_config.level = spdlog::level::debug;
#endif
    sandbox::logging::init(logging_config);

    if (glfwInit() == GLFW_FALSE) {
        LOG_CRITICAL("Failed to initialize GLFW");
        sandbox::logging::shutdown();
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Gloxide", nullptr, nullptr);
    if (window == nullptr) {
        LOG_CRITICAL("Failed to create GLFW window");
        glfwTerminate();
        sandbox::logging::shutdown();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSwapInterval(1);

    if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
        LOG_CRITICAL("Failed to load OpenGL functions");
        glfwDestroyWindow(window);
        glfwTerminate();
        sandbox::logging::shutdown();
        return 1;
    }

    const auto* version_string = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    LOG_INFO("Initialized OpenGL context: {}", version_string != nullptr ? version_string : "unknown");

    int width = 0;
    int height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    sandbox::AppContext app_context{};
    app_context.window = window;
    app_context.framebuffer_width = width;
    app_context.framebuffer_height = height;
    app_context.time_seconds = glfwGetTime();
    app_context.state_registry.register_state<sandbox::states::FragmentPlaygroundState>("Fragment Shader Playground");
    app_context.state_registry.register_state<sandbox::states::HelloCubeState>("Hello Cube");
    app_context.state_registry.register_state<sandbox::states::VoxelGameState>("Voxel Game");

    sandbox::StateManager state_manager{};
    state_manager.initialize<sandbox::states::SelectorMenuState>(app_context);

    double previous_time = app_context.time_seconds;
    bool shift_escape_was_down = false;

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glfwPollEvents();

        glfwGetFramebufferSize(window, &app_context.framebuffer_width, &app_context.framebuffer_height);

        app_context.time_seconds = glfwGetTime();
        const float delta_seconds = static_cast<float>(app_context.time_seconds - previous_time);
        previous_time = app_context.time_seconds;

        const bool shift_down = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                                glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
        const bool escape_down = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
        const bool shift_escape_down = shift_down && escape_down;

        if (shift_escape_down && !shift_escape_was_down) {
            app_context.pending_transition = sandbox::StateTransition::to<sandbox::states::SelectorMenuState>();
        }
        shift_escape_was_down = shift_escape_down;

        if (!state_manager.update(app_context, delta_seconds)) {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }

        glfwSwapBuffers(window);
    }

    state_manager.shutdown(app_context);

    glfwDestroyWindow(window);
    glfwTerminate();
    sandbox::logging::shutdown();
    return 0;
}
