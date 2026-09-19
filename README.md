# Geo Sandbox

A real-time engine sandbox using C++20 and Raylib for demonstrating geometric algorithms.

## Setup Instructions

### Prerequisites
- CMake 3.20 or higher
- A C++20 compatible compiler (e.g., MSVC, GCC, Clang)
- [vcpkg](https://vcpkg.io/) for dependency management (manifest mode is used).

### Building the Project

1. **Clone the repository**:
   ```bash
   git clone https://github.com/kozakbalint/geom
   cd geom
   ```

2. **Configure with CMake**:
   Make sure you have `vcpkg` installed. Set the toolchain file when configuring to resolve dependencies via `vcpkg.json`.
   ```bash
   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
   ```

3. **Build**:
   ```bash
   cmake --build build
   ```

4. **Run**:
   The executable will be generated in the `build` directory. Depending on your platform and generator (e.g. Visual Studio), you can run it via:
   ```bash
   ./build/Debug/geo_sandbox.exe
   ```

## How to Add a New Demo

To add a new isolated graphics demo to the sandbox, follow these steps:

1. **Create the Demo Class**:
   Create a new header and source file in `src/demos/` (e.g., `MyNewDemo.h` and `MyNewDemo.cpp`).
   Your class must inherit from the `Demo` abstract base class defined in `src/core/Demo.h` and implement the `init()`, `render()`, and `destroy()` methods.

   ```cpp
   // src/demos/MyNewDemo.h
   #pragma once
   #include "core/Demo.h"

   class MyNewDemo : public Demo {
   public:
       void init() override;
       void render() override;
       void destroy() override;
   };
   ```

2. **Register the Demo in CMake**:
   Add your new source file (`src/demos/MyNewDemo.cpp`) to the `add_executable` list in `CMakeLists.txt`.

3. **Integrate with the Sandbox Engine**:
   To make your demo selectable and renderable, you need to hook it into the `SandboxApp`.

   - **In `src/core/SandboxApp.h`**:
     - Include your demo's header: `#include "demos/MyNewDemo.h"`
     - Add an instance as a private member: `MyNewDemo myNewDemo;`

   - **In `src/core/SandboxApp.cpp`**:
     - **Initialization**: Inside `SandboxApp::initWindow()`, initialize your demo:
       ```cpp
       myNewDemo.init();
       ```
     - **Cleanup**: Inside `SandboxApp::cleanup()`, destroy your demo:
       ```cpp
       myNewDemo.destroy();
       ```
     - **UI Selection**: Inside `SandboxApp::renderUI()`, add your demo's name to the `items` array for the ImGui combo box. The index of the string in the array will become its demo index.
       ```cpp
       const char* items[] = { "Rainbow Triangle", "Catmull-Clark", "Tubular Lofting", "My New Demo" };
       ```
     - **Rendering**: Inside `SandboxApp::drawFrame()`, add a condition to render your demo when its index is selected:
       ```cpp
       if (currentDemoIndex == 3) { // Use the corresponding index from the items array
           myNewDemo.render();
       }
       ```
