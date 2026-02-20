# OpenGL Sandbox

Starter C++23 OpenGL 4.6 project using GLFW + GLAD with CMake.

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
