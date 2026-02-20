#pragma once

#include <string>
#include <string_view>

namespace sandbox::graphics {

std::string shader_file_path(std::string_view file_name);
std::string read_text_file(const std::string& file_path);

unsigned int compile_shader_from_source(unsigned int type, const std::string& source);
unsigned int create_program_from_sources(const std::string& vertex_source, const std::string& fragment_source);
unsigned int create_program_from_files(std::string_view vertex_shader_file,
                                       std::string_view fragment_shader_file);

} // namespace sandbox::graphics
