#pragma once

#include <cstddef>
#include <source_location>
#include <string_view>
#include <utility>

#include <spdlog/common.h>
#include <spdlog/fmt/fmt.h>

namespace sandbox::logging {

enum class Backend {
    spdlog_console,
    noop,
};

struct Config {
    Backend backend = Backend::spdlog_console;
    bool asynchronous = true;
    bool non_blocking = true;
    std::size_t queue_size = 8192;
    std::size_t worker_threads = 1;
    std::size_t backtrace_messages = 64;
    spdlog::level::level_enum level = spdlog::level::debug;
};

void init(const Config& config = {});
void shutdown();
void install_terminate_handler();
void dump_backtrace();

void log(spdlog::level::level_enum level,
         std::string_view message,
         const std::source_location& location = std::source_location::current());

template <typename... Args>
inline void trace(const std::source_location& location, spdlog::format_string_t<Args...> fmt, Args&&... args) {
    log(spdlog::level::trace, fmt::format(fmt, std::forward<Args>(args)...), location);
}

template <typename... Args>
inline void debug(const std::source_location& location, spdlog::format_string_t<Args...> fmt, Args&&... args) {
    log(spdlog::level::debug, fmt::format(fmt, std::forward<Args>(args)...), location);
}

template <typename... Args>
inline void info(const std::source_location& location, spdlog::format_string_t<Args...> fmt, Args&&... args) {
    log(spdlog::level::info, fmt::format(fmt, std::forward<Args>(args)...), location);
}

template <typename... Args>
inline void warn(const std::source_location& location, spdlog::format_string_t<Args...> fmt, Args&&... args) {
    log(spdlog::level::warn, fmt::format(fmt, std::forward<Args>(args)...), location);
}

template <typename... Args>
inline void error(const std::source_location& location, spdlog::format_string_t<Args...> fmt, Args&&... args) {
    log(spdlog::level::err, fmt::format(fmt, std::forward<Args>(args)...), location);
}

template <typename... Args>
inline void critical(const std::source_location& location,
                     spdlog::format_string_t<Args...> fmt,
                     Args&&... args) {
    log(spdlog::level::critical, fmt::format(fmt, std::forward<Args>(args)...), location);
}

} // namespace sandbox::logging

#define LOG_TRACE(...) ::sandbox::logging::trace(std::source_location::current(), __VA_ARGS__)
#define LOG_DEBUG(...) ::sandbox::logging::debug(std::source_location::current(), __VA_ARGS__)
#define LOG_INFO(...) ::sandbox::logging::info(std::source_location::current(), __VA_ARGS__)
#define LOG_WARN(...) ::sandbox::logging::warn(std::source_location::current(), __VA_ARGS__)
#define LOG_ERROR(...) ::sandbox::logging::error(std::source_location::current(), __VA_ARGS__)
#define LOG_CRITICAL(...) ::sandbox::logging::critical(std::source_location::current(), __VA_ARGS__)
