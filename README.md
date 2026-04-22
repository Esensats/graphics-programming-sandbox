# OpenGL Sandbox

![Voxel Game State](./screenshots/voxel_game_state.png)

Starter C++23 OpenGL 4.6 project using GLFW + GLAD with CMake. Includes a simple state management system for easily creating and switching between different graphical experiments. The project is designed to be a sandbox for learning and prototyping graphics programming concepts without the overhead of setting up the boilerplate code every time. Screenshots at [the bottom](#screenshots) show some example states that have been implemented, including a fragment shader playground and a simple voxel engine.

## Abstract

When starting to learn graphics programming, beginners face a significant barrier to entry. Before you can even begin to explore the most interesting parts—such as shaders, lighting models, physics, and simulations—you are forced to navigate a maze of boilerplate. This includes:
- Choosing the right combination of libraries and technologies
- Setting up the build system (e.g., CMake)
- Handling window creation and OpenGL context initialization
- Designing application architecture and the main loop
- Building out utility pipelines for loading and compiling shaders

Every time you want to build a new graphics experiment, you often have to rewrite or copy-paste this tedious setup all over again.

Many solutions exist to circumvent this: web-based environments like Shadertoy are excellent but restrict you to fragment shaders and hide the host-side C++ architecture. Full-scale game engines like Unity or Unreal are incredibly powerful but abstract the underlying graphics API away entirely, defeating the purpose of learning the low-level mechanics.

**OpenGL Sandbox** bridges this gap. It is a lightweight, pre-configured C++23 framework that takes care of the boilerplate (GLFW, GLAD, ImGui, and core utilities) so you can focus on the fun parts. Using a simple [State pattern](https://refactoring.guru/design-patterns/state), you can seamlessly create isolated graphical experiments—ranging from simple 2D fragment shader playgrounds to complex multi-chunk 3D voxel engines—all within the same project. 

It's simple to get started:

1. Fork the repo
2. Look at how the example states are implemented
3. Write your own experiments!

## Requirements

- CMake 3.20+
- C++ compiler with C++23 support
- Python 3 (used by GLAD code generation during CMake configure/build)
- OpenGL development libraries (Linux)

## Clone and initialize submodules

```bash
git submodule update --init --depth 1 --recursive
```

## Build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

## Run

```bash
./build/opengl_sandbox
```

## Screenshots

![State selector menu state](./screenshots/state_selector_menu.png)
![Fragment shader playground state](./screenshots/fragment_shader_playground_state.png)
![Hello cube state](./screenshots/hello_cube_state.png)

## Documentation

- [Voxel Engine / Game Master Plan](docs/voxel-engine-planning.md)

