#include "sandbox/states/fragment_playground_state.hpp"

#include <array>
#include <string>

#include <glad/gl.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

unsigned int compile_shader(unsigned int type, const char* source) {
    const unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    int success = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE) {
        int info_log_length = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(static_cast<std::size_t>(info_log_length), '\0');
        glGetShaderInfoLog(shader, info_log_length, nullptr, info_log.data());
        LOG_ERROR("Shader compilation failed: {}", info_log);
        glDeleteShader(shader);
        return 0;
    }

    return shader;
}

unsigned int create_program(const char* vertex_source, const char* fragment_source) {
    const unsigned int vertex_shader = compile_shader(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) {
        return 0;
    }

    const unsigned int fragment_shader = compile_shader(GL_FRAGMENT_SHADER, fragment_source);
    if (fragment_shader == 0) {
        glDeleteShader(vertex_shader);
        return 0;
    }

    const unsigned int program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    int success = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (success == GL_FALSE) {
        int info_log_length = 0;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_log_length);
        std::string info_log(static_cast<std::size_t>(info_log_length), '\0');
        glGetProgramInfoLog(program, info_log_length, nullptr, info_log.data());
        LOG_ERROR("Program link failed: {}", info_log);
        glDeleteProgram(program);
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        return 0;
    }

    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return program;
}

} // namespace

void FragmentPlaygroundState::on_enter(AppContext& context) {
    (void)context;

    constexpr auto k_vertex_source = R"(
#version 460 core

const vec2 positions[3] = vec2[](
    vec2(-1.0, -1.0),
    vec2(3.0, -1.0),
    vec2(-1.0, 3.0)
);

void main() {
    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);
}
)";

    constexpr auto k_fragment_source = R"(
#version 460 core

out vec4 FragColor;
uniform vec2 u_resolution;
uniform float u_time;

void main() {
    vec2 uv = gl_FragCoord.xy / u_resolution.xy;
    vec2 centered = uv * 2.0 - 1.0;
    centered.x *= u_resolution.x / u_resolution.y;

    float radial = length(centered);
    float waves = sin(10.0 * radial - u_time * 2.0);
    vec3 base = vec3(0.1, 0.2, 0.8);
    vec3 accent = vec3(0.9, 0.2, 0.6);
    vec3 color = mix(base, accent, 0.5 + 0.5 * waves);
    color *= 1.0 - smoothstep(0.2, 1.2, radial);

    FragColor = vec4(color, 1.0);
}
)";

    program_ = create_program(k_vertex_source, k_fragment_source);
    glGenVertexArrays(1, &vao_);
    LOG_INFO("Entered fragment shader playground state");
}

void FragmentPlaygroundState::on_exit(AppContext& context) {
    (void)context;

    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    LOG_INFO("Exited fragment shader playground state");
}

StateTransition FragmentPlaygroundState::update(AppContext& context, float delta_seconds) {
    (void)delta_seconds;

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.03f, 0.03f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (program_ == 0 || vao_ == 0) {
        return StateTransition::none();
    }

    glUseProgram(program_);

    const int resolution_loc = glGetUniformLocation(program_, "u_resolution");
    glUniform2f(resolution_loc, static_cast<float>(context.framebuffer_width),
                static_cast<float>(context.framebuffer_height));

    const int time_loc = glGetUniformLocation(program_, "u_time");
    glUniform1f(time_loc, static_cast<float>(context.time_seconds));

    glBindVertexArray(vao_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    return StateTransition::none();
}

} // namespace sandbox::states
