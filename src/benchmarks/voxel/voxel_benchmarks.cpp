#include "benchmarks/voxel/benchmark_support.hpp"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <tuple>
#include <vector>

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "sandbox/logging.hpp"
#include "sandbox/voxel/meshing/controller.hpp"
#include "sandbox/voxel/render/render_system.hpp"
#include "sandbox/voxel/runtime.hpp"
#include "sandbox/voxel/streaming/residency.hpp"
#include "sandbox/voxel/streaming/terrain_generator.hpp"
#include "sandbox/voxel/world/world.hpp"

namespace sandbox::benchmarks::voxel {
namespace {

namespace vxl = ::sandbox::voxel;

#ifndef SANDBOX_RESOURCE_PACK_DIR
#define SANDBOX_RESOURCE_PACK_DIR "resource_packs"
#endif

struct HiddenOpenGlContext {
    GLFWwindow* window = nullptr;

    bool initialize() {
        if (glfwInit() == GLFW_FALSE) {
            return false;
        }

        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        window = glfwCreateWindow(320, 240, "voxel_benchmarks", nullptr, nullptr);
        if (window == nullptr) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);
        if (gladLoadGL(reinterpret_cast<GLADloadfunc>(glfwGetProcAddress)) == 0) {
            glfwDestroyWindow(window);
            window = nullptr;
            glfwTerminate();
            return false;
        }

        return true;
    }

    void shutdown() {
        if (window != nullptr) {
            glfwDestroyWindow(window);
            window = nullptr;
        }
        glfwTerminate();
    }
};

constexpr std::size_t k_benchmark_worker_count = 2;
constexpr int k_warmup_frames = 180;
constexpr int k_drain_frame_budget = 1'200;
constexpr int k_drain_idle_frames = 12;

[[nodiscard]] bool drain_residency(vxl::streaming::ResidencyController& residency,
                                   vxl::world::World& world,
                                   int max_frames,
                                   int idle_frames_required) {
    int idle_frames = 0;
    for (int frame = 0; frame < max_frames; ++frame) {
        residency.update(world);
        const bool queues_empty = residency.queued_generation_count() == 0
            && residency.queued_unload_count() == 0;
        if (queues_empty) {
            ++idle_frames;
            if (idle_frames >= idle_frames_required) {
                return true;
            }
        } else {
            idle_frames = 0;
        }
    }

    return false;
}

[[nodiscard]] bool drain_meshing(vxl::meshing::Controller& meshing,
                                 vxl::world::World& world,
                                 int max_frames,
                                 int idle_frames_required) {
    int idle_frames = 0;
    for (int frame = 0; frame < max_frames; ++frame) {
        meshing.update(world);
        const bool queues_empty = meshing.queued_build_count() == 0
            && meshing.queued_upload_count() == 0;
        if (queues_empty) {
            ++idle_frames;
            if (idle_frames >= idle_frames_required) {
                return true;
            }
        } else {
            idle_frames = 0;
        }
    }

    return false;
}

[[nodiscard]] bool drain_runtime(vxl::Runtime& runtime,
                                 int max_frames,
                                 int idle_frames_required) {
    int idle_frames = 0;
    for (int frame = 0; frame < max_frames; ++frame) {
        runtime.update_frame(1.0f / 60.0f);
        const vxl::RuntimeDebugSnapshot snapshot = runtime.debug_snapshot();
        const bool queues_empty = snapshot.generation_queued_count == 0
            && snapshot.upload_queued_count == 0;
        if (queues_empty) {
            ++idle_frames;
            if (idle_frames >= idle_frames_required) {
                return true;
            }
        } else {
            idle_frames = 0;
        }
    }

    return false;
}

[[nodiscard]] vxl::world::World seed_world_with_terrain(int half_extent_x,
                                                        int half_extent_y,
                                                        int half_extent_z,
                                                        std::uint32_t seed) {
    vxl::world::World world{};
    vxl::streaming::TerrainGenerator generator(seed);

    for (int z = -half_extent_z; z <= half_extent_z; ++z) {
        for (int y = -half_extent_y; y <= half_extent_y; ++y) {
            for (int x = -half_extent_x; x <= half_extent_x; ++x) {
                const vxl::world::ChunkKey key{x, y, z};
                vxl::world::Chunk& chunk = world.ensure_chunk(key);
                generator.populate_chunk(key, chunk);
            }
        }
    }

    return world;
}

BenchmarkRun benchmark_terrain_generation() {
    vxl::streaming::TerrainGenerator generator(1337);
    vxl::world::Chunk chunk{};
    int tick = 0;

    return run_benchmark("terrain_generation/populate_chunk", 16'000, [&tick, &generator, &chunk]() {
        const vxl::world::ChunkKey key{tick % 64, (tick / 64) % 8, (tick / 8) % 64};
        generator.populate_chunk(key, chunk);
        ++tick;
    });
}

BenchmarkRun benchmark_chunk_meshing() {
    vxl::streaming::TerrainGenerator generator(1337);
    vxl::world::Chunk seeded{};
    generator.populate_chunk(vxl::world::ChunkKey{2, 1, -3}, seeded);

    vxl::meshing::Controller::BuildRequest request{};
    request.key = vxl::world::ChunkKey{2, 1, -3};
    request.blocks = seeded.blocks();

    return run_benchmark("chunk_meshing/build_chunk_mesh", 20'000, [&]() {
        const vxl::meshing::ChunkMeshInfo mesh = vxl::meshing::Controller::build_chunk_mesh(request);
        (void)mesh;
    });
}

BenchmarkRun benchmark_residency_controller() {
    vxl::world::World world{};
    vxl::streaming::ResidencyController residency{};

    residency.initialize(vxl::streaming::StreamingConfig{
        .horizontal_radius_chunks = 3,
        .vertical_radius_chunks = 1,
        .generation_budget_per_frame = 24,
        .unload_budget_per_frame = 24,
        .generation_workers = k_benchmark_worker_count,
        .seed = 1337,
    });

    int frame = 0;
    for (int warmup = 0; warmup < k_warmup_frames; ++warmup) {
        const vxl::world::WorldVoxelCoord focus{frame * 2, 0, frame};
        residency.set_focus_world(focus);
        residency.update(world);
        ++frame;
    }

    (void)drain_residency(residency, world, k_drain_frame_budget, k_drain_idle_frames);

    BenchmarkRun result = run_benchmark("residency_controller/update", 1e5, [&]() {
        const vxl::world::WorldVoxelCoord focus{frame * 2, 0, frame};
        residency.set_focus_world(focus);
        residency.update(world);
        ++frame;
    });

    (void)drain_residency(residency, world, k_drain_frame_budget, k_drain_idle_frames);

    residency.shutdown();
    return result;
}

BenchmarkRun benchmark_meshing_controller() {
    vxl::world::World world = seed_world_with_terrain(3, 1, 3, 9001);
    vxl::meshing::Controller controller{};

    std::vector<vxl::world::ChunkKey> sorted_keys = world.chunk_keys();
    std::sort(sorted_keys.begin(), sorted_keys.end(), [](const vxl::world::ChunkKey& lhs, const vxl::world::ChunkKey& rhs) {
        return std::tie(lhs.x, lhs.y, lhs.z) < std::tie(rhs.x, rhs.y, rhs.z);
    });

    for (const vxl::world::ChunkKey& key : sorted_keys) {
        vxl::world::Chunk* chunk = world.find_chunk(key);
        if (chunk != nullptr) {
            chunk->mark_dirty_mesh();
        }
    }

    controller.initialize(vxl::meshing::MeshingConfig{
        .workers = k_benchmark_worker_count,
        .build_commit_budget_per_frame = 48,
        .upload_budget_per_frame = 64,
    });

    for (int warmup = 0; warmup < k_warmup_frames; ++warmup) {
        controller.update(world);
    }

    (void)drain_meshing(controller, world, k_drain_frame_budget, k_drain_idle_frames);
    std::size_t chunk_index = 0;

    BenchmarkRun result = run_benchmark("meshing_controller/update", 3e4, [&]() {
        if (!sorted_keys.empty()) {
            const vxl::world::ChunkKey key = sorted_keys[chunk_index % sorted_keys.size()];
            ++chunk_index;
            vxl::world::Chunk* chunk = world.find_chunk(key);
            if (chunk != nullptr) {
                chunk->mark_dirty_mesh();
            }
        }

        controller.update(world);
    });

    (void)drain_meshing(controller, world, k_drain_frame_budget, k_drain_idle_frames);

    controller.shutdown();
    return result;
}

BenchmarkRun benchmark_end_to_end_pipeline() {
    vxl::Runtime runtime{};
    runtime.initialize(vxl::RuntimeConfig{
        .streaming = vxl::streaming::StreamingConfig{
            .horizontal_radius_chunks = 2,
            .vertical_radius_chunks = 1,
            .generation_budget_per_frame = 16,
            .unload_budget_per_frame = 16,
            .generation_workers = k_benchmark_worker_count,
            .seed = 1337,
        },
        .meshing = vxl::meshing::MeshingConfig{
            .workers = k_benchmark_worker_count,
            .build_commit_budget_per_frame = 32,
            .upload_budget_per_frame = 32,
        },
    });

    vxl::render::RenderSystem renderer{};
    renderer.initialize(SANDBOX_RESOURCE_PACK_DIR "/default");

    const glm::vec3 eye{0.0f, 32.0f, 96.0f};
    const glm::mat4 projection = glm::perspective(glm::radians(75.0f), 1280.0f / 720.0f, 0.1f, 1000.0f);
    const glm::mat4 view = glm::lookAt(eye, glm::vec3{0.0f, 32.0f, 0.0f}, glm::vec3{0.0f, 1.0f, 0.0f});

    for (int warmup = 0; warmup < k_warmup_frames; ++warmup) {
        runtime.update_frame(1.0f / 60.0f);
    }

    (void)drain_runtime(runtime, k_drain_frame_budget, k_drain_idle_frames);

    BenchmarkRun result = run_benchmark("end_to_end/frame", 300, [&]() {
        runtime.update_frame(1.0f / 60.0f);

        const auto draws = runtime.visible_draw_lists(vxl::meshing::VisibilityQuery{
            .origin_world = vxl::world::WorldVoxelCoord{0, 32, 96},
            .enable_distance_cull = true,
            .max_chunk_distance = 8,
            .enable_frustum_cull = false,
        });

        renderer.render_frame(vxl::render::RenderFrameInput{
            .framebuffer_width = 1280,
            .framebuffer_height = 720,
            .camera_world = eye,
            .view_projection = projection * view,
            .draw_lists = draws,
        });

        glFinish();
    });

    (void)drain_runtime(runtime, k_drain_frame_budget, k_drain_idle_frames);

    renderer.shutdown();
    runtime.shutdown();
    return result;
}

} // namespace

} // namespace sandbox::benchmarks::voxel

int main() {
#if !defined(NDEBUG)
    std::fputs("voxel_benchmarks: run a Release build for stable benchmark numbers\n", stderr);
    return EXIT_FAILURE;
#endif

    sandbox::logging::Config logging_config{};
    logging_config.level = spdlog::level::warn;
    sandbox::logging::init(logging_config);

    sandbox::benchmarks::voxel::HiddenOpenGlContext context{};
    if (!context.initialize()) {
        std::fputs("Failed to initialize hidden OpenGL context for benchmarks\n", stderr);
        sandbox::logging::shutdown();
        return EXIT_FAILURE;
    }

    std::vector<sandbox::benchmarks::voxel::BenchmarkRun> runs;
    runs.reserve(5);

    runs.push_back(sandbox::benchmarks::voxel::benchmark_terrain_generation());
    runs.push_back(sandbox::benchmarks::voxel::benchmark_chunk_meshing());

    runs.push_back(sandbox::benchmarks::voxel::benchmark_residency_controller());
    runs.push_back(sandbox::benchmarks::voxel::benchmark_meshing_controller());
    runs.push_back(sandbox::benchmarks::voxel::benchmark_end_to_end_pipeline());

    sandbox::benchmarks::voxel::print_results(runs);

    context.shutdown();
    sandbox::logging::shutdown();
    return EXIT_SUCCESS;
}
