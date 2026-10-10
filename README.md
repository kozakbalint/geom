# Geo sandbox

A real-time engine sandbox using C++20 and Raylib for demonstrating geometric algorithms.

## Setup instructions

For a complete walkthrough covering Visual Studio, VS Code, automated scripts, and troubleshooting, refer to [SETUP.md](SETUP.md).

### Prerequisites

- CMake 3.20 or higher.
- C++20 compatible compiler such as MSVC, Clang, or GCC.
- Git.

### Quick start

1. Clone the repository.
   ```bash
   git clone https://github.com/kozakbalint/geom
   cd geom
   ```

2. Configure CMake. External dependencies download automatically.
   ```bash
   cmake -B build -S .
   ```

3. Build the project.
   ```bash
   cmake --build build --config Debug
   ```

4. Run the executable.
   ```bash
   ./build/Debug/geo_sandbox.exe
   ```

You can also use the PowerShell build helper on Windows:
```powershell
.\scripts\build-windows.ps1
```

## How to add a new demo

Follow these steps to add a new isolated graphics demo to the sandbox:

1. Create the demo class.
   Create a new header and source file in `src/demos/` (such as `MyNewDemo.h` and `MyNewDemo.cpp`).
   Your class must inherit from the `Demo` abstract base class defined in `src/demos/Demo.h`.
   Implement the `init()`, `render()`, and `destroy()` methods.
   You can also optionally override `renderUI()`.

   ```cpp
   // src/demos/MyNewDemo.h
   #pragma once
   #include "demos/Demo.h"

   class MyNewDemo : public Demo {
   public:
       void init() override;
       void render() override;
       void renderUI() override; // optional
       void destroy() override;
   };
   ```

2. Register the demo in CMake.
   Add your new source file (`src/demos/MyNewDemo.cpp`) to the `add_executable` list in `CMakeLists.txt`.

3. Integrate with the sandbox app.
   Hook your demo into `SandboxApp` to make it selectable in the UI.

   - In `src/app/SandboxApp.h`:
     - Include your demo header:
       ```cpp
       #include "demos/MyNewDemo.h"
       ```
     - Add an instance as a private member:
       ```cpp
       MyNewDemo myNewDemo;
       ```

   - In `src/app/SandboxApp.cpp`:
     - Inside `SandboxApp::initWindow()`, initialize your demo:
       ```cpp
       myNewDemo.init();
       ```
     - Inside `SandboxApp::cleanup()`, destroy your demo:
       ```cpp
       myNewDemo.destroy();
       ```
     - Inside `SandboxApp::renderUI()`, add your demo name to the `items` array for the ImGui combo box:
       ```cpp
       const char* items[] = { "Subdivision", "My New Demo" };
       ```
       Call its UI rendering method when selected:
       ```cpp
       if (currentDemoIndex == 1) {
           myNewDemo.renderUI();
       }
       ```
     - Inside `SandboxApp::drawFrame()`, add a condition to render your demo when selected:
       ```cpp
       if (currentDemoIndex == 1) {
           myNewDemo.render();
       }
       ```
