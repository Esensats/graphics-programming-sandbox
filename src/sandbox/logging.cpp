#include "sandbox/logging.hpp"

#include <spdlog/async.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <sstream>

#if defined(__has_include)
#if __has_include(<stacktrace>)
#include <stacktrace>
#define SANDBOX_HAS_STACKTRACE 1
#else
#define SANDBOX_HAS_STACKTRACE 0
#endif
#else
#define SANDBOX_HAS_STACKTRACE 0
#endif

namespace sandbox::logging {

namespace {

constexpr const char* k_logger_name = "sandbox";

struct LoggingState {
    Backend backend = Backend::spdlog_console;
    bool initialized = false;
};

LoggingState& state() {
    static LoggingState instance;
    return instance;
}

std::atomic_bool& terminate_handler_installed() {
    static std::atomic_bool installed{false};
    return installed;
}

std::string stacktrace_to_string() {
#if SANDBOX_HAS_STACKTRACE
    std::ostringstream stream;
    stream << std::stacktrace::current();
    return stream.str();
#else
    return "Stacktrace unavailable (standard <stacktrace> not supported by this toolchain).";
#endif
}

std::shared_ptr<spdlog::logger> make_spdlog_logger(const Config& config) {
    if (config.asynchronous) {
        spdlog::init_thread_pool(config.queue_size, config.worker_threads);
        if (config.non_blocking) {
            return spdlog::create_async_nb<spdlog::sinks::stdout_color_sink_mt>(k_logger_name);
        }
        return spdlog::create_async<spdlog::sinks::stdout_color_sink_mt>(k_logger_name);
    }

    auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    return std::make_shared<spdlog::logger>(k_logger_name, std::move(sink));
}

void configure_spdlog_logger(const std::shared_ptr<spdlog::logger>& logger, const Config& config) {
    logger->set_level(config.level);
    logger->flush_on(spdlog::level::warn);
    logger->set_pattern("[%H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");

    spdlog::set_default_logger(logger);
    spdlog::set_level(config.level);
    spdlog::flush_on(spdlog::level::warn);
    spdlog::enable_backtrace(config.backtrace_messages);
    spdlog::set_error_handler([](const std::string& message) {
        std::fprintf(stderr, "spdlog internal error: %s\n", message.c_str());
    });
}

void log_to_backend(spdlog::level::level_enum level,
                    std::string_view message,
                    const std::source_location& location) {
    const auto current_backend = state().backend;
    if (current_backend == Backend::noop) {
        return;
    }

    auto* logger = spdlog::default_logger_raw();
    if (logger == nullptr) {
        return;
    }

    const auto source = spdlog::source_loc{location.file_name(), static_cast<int>(location.line()), location.function_name()};
    logger->log(source, level, "{}", message);
}

} // namespace

void init(const Config& config) {
    shutdown();
    state().backend = config.backend;

    if (config.backend == Backend::noop) {
        state().initialized = true;
        return;
    }

    auto logger = make_spdlog_logger(config);
    configure_spdlog_logger(logger, config);
    state().initialized = true;
    install_terminate_handler();

    log(spdlog::level::info, "Logger initialized (backend: spdlog)");
}

void shutdown() {
    if (!state().initialized) {
        return;
    }

    if (state().backend == Backend::spdlog_console) {
        log(spdlog::level::info, "Logger shutdown");
    }

    spdlog::shutdown();
    state().initialized = false;
}

void install_terminate_handler() {
    bool expected = false;
    if (!terminate_handler_installed().compare_exchange_strong(expected, true)) {
        return;
    }

    std::set_terminate([] {
        if (const auto exception = std::current_exception()) {
            try {
                std::rethrow_exception(exception);
            } catch (const std::exception& ex) {
                log(spdlog::level::critical, fmt::format("Unhandled exception: {}", ex.what()));
            } catch (...) {
                log(spdlog::level::critical, "Unhandled non-standard exception");
            }
        } else {
            log(spdlog::level::critical, "Program terminated without active exception");
        }

        log(spdlog::level::critical, "Stacktrace:", std::source_location::current());
        log(spdlog::level::critical, stacktrace_to_string(), std::source_location::current());
        dump_backtrace();
        shutdown();
        std::abort();
    });
}

void dump_backtrace() {
    if (state().backend == Backend::noop) {
        return;
    }
    spdlog::dump_backtrace();
}

void log(spdlog::level::level_enum level, std::string_view message, const std::source_location& location) {
    if (!state().initialized) {
        return;
    }

    log_to_backend(level, message, location);
}

} // namespace sandbox::logging
