#include "sandbox/states/hello_cube_state.hpp"

#include <array>
#include <cmath>
#include <cstddef>
#include <string>

#include <glad/gl.h>

#include "sandbox/app_context.hpp"
#include "sandbox/logging.hpp"

namespace sandbox::states {
namespace {

struct Mat4 {
    std::array<float, 16> values{};
};

Mat4 mat4_identity() {
    return Mat4{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
}

Mat4 mat4_multiply(const Mat4& lhs, const Mat4& rhs) {
    Mat4 result{};
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += lhs.values[static_cast<std::size_t>(k * 4 + row)] *
                       rhs.values[static_cast<std::size_t>(col * 4 + k)];
            }
            result.values[static_cast<std::size_t>(col * 4 + row)] = sum;
        }
    }
    return result;
}

Mat4 mat4_translation(float x, float y, float z) {
    Mat4 result = mat4_identity();
    result.values[12] = x;
    result.values[13] = y;
    result.values[14] = z;
    return result;
}

Mat4 mat4_rotation_x(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c, s, 0.0f,
        0.0f, -s, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
}

Mat4 mat4_rotation_y(float radians) {
    const float c = std::cos(radians);
    const float s = std::sin(radians);
    return Mat4{{
        c, 0.0f, -s, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        s, 0.0f, c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f,
    }};
}

Mat4 mat4_perspective(float fov_radians, float aspect, float z_near, float z_far) {
    const float tan_half_fov = std::tan(fov_radians * 0.5f);
    Mat4 result{};
    result.values[0] = 1.0f / (aspect * tan_half_fov);
    result.values[5] = 1.0f / tan_half_fov;
    result.values[10] = -(z_far + z_near) / (z_far - z_near);
    result.values[11] = -1.0f;
    result.values[14] = -(2.0f * z_far * z_near) / (z_far - z_near);
    return result;
}

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

void HelloCubeState::on_enter(AppContext& context) {
    (void)context;

    constexpr auto k_vertex_source = R"(
#version 460 core

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_color;

uniform mat4 u_mvp;
out vec3 v_color;

void main() {
    gl_Position = u_mvp * vec4(a_position, 1.0);
    v_color = a_color;
}
)";

    constexpr auto k_fragment_source = R"(
#version 460 core

in vec3 v_color;
out vec4 FragColor;

void main() {
    FragColor = vec4(v_color, 1.0);
}
)";

    constexpr std::array<float, 48> vertices = {
        -0.5f, -0.5f, -0.5f, 1.0f, 0.2f, 0.2f,
         0.5f, -0.5f, -0.5f, 0.2f, 1.0f, 0.2f,
         0.5f,  0.5f, -0.5f, 0.2f, 0.2f, 1.0f,
        -0.5f,  0.5f, -0.5f, 1.0f, 1.0f, 0.2f,
        -0.5f, -0.5f,  0.5f, 1.0f, 0.2f, 1.0f,
         0.5f, -0.5f,  0.5f, 0.2f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f, 1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f, 0.3f, 0.3f, 0.3f,
    };

    constexpr std::array<unsigned int, 36> indices = {
        0, 1, 2, 2, 3, 0,
        1, 5, 6, 6, 2, 1,
        5, 4, 7, 7, 6, 5,
        4, 0, 3, 3, 7, 4,
        3, 2, 6, 6, 7, 3,
        4, 5, 1, 1, 0, 4,
    };

    program_ = create_program(k_vertex_source, k_fragment_source);

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    glBindVertexArray(vao_);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(float)),
                 vertices.data(),
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
                 indices.data(),
                 GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * static_cast<int>(sizeof(float)), nullptr);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          6 * static_cast<int>(sizeof(float)),
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);

    elapsed_seconds_ = 0.0f;
    LOG_INFO("Entered hello cube state");
}

void HelloCubeState::on_exit(AppContext& context) {
    (void)context;

    if (ebo_ != 0) {
        glDeleteBuffers(1, &ebo_);
        ebo_ = 0;
    }

    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    if (vao_ != 0) {
        glDeleteVertexArrays(1, &vao_);
        vao_ = 0;
    }

    if (program_ != 0) {
        glDeleteProgram(program_);
        program_ = 0;
    }

    glDisable(GL_DEPTH_TEST);
    LOG_INFO("Exited hello cube state");
}

StateTransition HelloCubeState::update(AppContext& context, float delta_seconds) {
    elapsed_seconds_ += delta_seconds;

    glEnable(GL_DEPTH_TEST);
    glViewport(0, 0, context.framebuffer_width, context.framebuffer_height);
    glClearColor(0.05f, 0.06f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (program_ == 0 || vao_ == 0) {
        return StateTransition::none();
    }

    const float aspect = static_cast<float>(context.framebuffer_width) /
                         static_cast<float>(context.framebuffer_height > 0 ? context.framebuffer_height : 1);

    const Mat4 projection = mat4_perspective(0.95f, aspect, 0.1f, 100.0f);
    const Mat4 view = mat4_translation(0.0f, 0.0f, -2.5f);
    const Mat4 model = mat4_multiply(mat4_rotation_y(elapsed_seconds_), mat4_rotation_x(elapsed_seconds_ * 0.7f));
    const Mat4 mvp = mat4_multiply(projection, mat4_multiply(view, model));

    glUseProgram(program_);
    const int mvp_loc = glGetUniformLocation(program_, "u_mvp");
    glUniformMatrix4fv(mvp_loc, 1, GL_FALSE, mvp.values.data());

    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    return StateTransition::none();
}

} // namespace sandbox::states
