#include "benchmarks/voxel/benchmark_support.hpp"

#include <cstdio>

namespace sandbox::benchmarks::voxel {

BenchmarkRun run_benchmark(const std::string& name,
                           std::size_t iterations,
                           const BenchmarkBody& body) {
    const auto start = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < iterations; ++index) {
        body();
    }
    const auto end = std::chrono::steady_clock::now();

    return BenchmarkRun{
        .name = name,
        .iterations = iterations,
        .total_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start),
    };
}

void print_results(const std::vector<BenchmarkRun>& runs) {
    std::puts("\nVoxel Benchmarks");
    std::puts("---------------------------------------------------------------");
    std::puts("name                                     total ms    avg us/op");
    std::puts("---------------------------------------------------------------");

    for (const BenchmarkRun& run : runs) {
        const double total_ms = static_cast<double>(run.total_time.count()) / 1'000'000.0;
        const double avg_us = run.iterations > 0
            ? static_cast<double>(run.total_time.count()) / static_cast<double>(run.iterations) / 1'000.0
            : 0.0;

        std::printf("%-40s %10.3f %12.3f\n", run.name.c_str(), total_ms, avg_us);
    }

    std::puts("---------------------------------------------------------------");
}

} // namespace sandbox::benchmarks::voxel
