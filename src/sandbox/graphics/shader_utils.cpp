#include "sandbox/graphics/shader_utils.hpp"

#include <cstddef>
#include <fstream>
#include <sstream>
#include <string>

#include <glad/gl.h>

#include "sandbox/logging.hpp"

namespace sandbox::graphics {
namespace {

#ifndef SANDBOX_SHADER_DIR
#define SANDBOX_SHADER_DIR "shaders"
#endif

} // namespace

std::string shader_file_path(std::string_view file_name) {
    return std::string(SANDBOX_SHADER_DIR) + "/" + std::string(file_name);
}

std::string read_text_file(const std::string& file_path) {
    std::ifstream file(file_path);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open file: {}", file_path);
        return {};
    }

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}

unsigned int compile_shader_from_source(unsigned int type, const std::string& source) {
    const unsigned int shader = glCreateShader(type);
    const char* source_ptr = source.c_str();
    glShaderSource(shader, 1, &source_ptr, nullptr);
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

unsigned int create_program_from_sources(const std::string& vertex_source, const std::string& fragment_source) {
    const unsigned int vertex_shader = compile_shader_from_source(GL_VERTEX_SHADER, vertex_source);
    if (vertex_shader == 0) {
        return 0;
    }

    const unsigned int fragment_shader = compile_shader_from_source(GL_FRAGMENT_SHADER, fragment_source);
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

unsigned int create_program_from_files(std::string_view vertex_shader_file,
                                       std::string_view fragment_shader_file) {
    const std::string vertex_source = read_text_file(shader_file_path(vertex_shader_file));
    const std::string fragment_source = read_text_file(shader_file_path(fragment_shader_file));
    if (vertex_source.empty() || fragment_source.empty()) {
        return 0;
    }

    return create_program_from_sources(vertex_source, fragment_source);
}

} // namespace sandbox::graphics
