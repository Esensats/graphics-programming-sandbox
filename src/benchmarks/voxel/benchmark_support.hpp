#pragma once

#include <chrono>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

namespace sandbox::benchmarks::voxel {

struct BenchmarkRun {
    std::string name{};
    std::size_t iterations = 0;
    std::chrono::nanoseconds total_time{};
};

using BenchmarkBody = std::function<void()>;

[[nodiscard]] BenchmarkRun run_benchmark(const std::string& name,
                                         std::size_t iterations,
                                         const BenchmarkBody& body);
void print_results(const std::vector<BenchmarkRun>& runs);

} // namespace sandbox::benchmarks::voxel
