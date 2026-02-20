#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "sandbox/logging.hpp"

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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "[floating] OpenGL Sandbox", nullptr, nullptr);
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

    while (glfwWindowShouldClose(window) == GLFW_FALSE) {
        glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    sandbox::logging::shutdown();
    return 0;
}
