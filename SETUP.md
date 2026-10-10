# Windows setup guide

This guide covers building Geo Sandbox on Windows 10 and Windows 11.

## System requirements

- Windows 10 or Windows 11 (64-bit).
- Graphics card with OpenGL 3.3 support or higher.

## Prerequisites

Install the required tools through `winget`.

Install Git:
```powershell
winget install --id Git.Git -e --source winget
```

Install Visual Studio 2022 Community with the C++ workload:
```powershell
winget install --id Microsoft.VisualStudio.2022.Community --override "--passive --add Microsoft.VisualStudio.Workload.NativeDesktop --includeRecommended"
```

Install standalone CMake (optional if using Visual Studio bundled tools):
```powershell
winget install --id Kitware.CMake -e --source winget
```

Install Ninja (optional build generator):
```powershell
winget install --id Ninja-build.Ninja -e --source winget
```

## Build workflows

Choose one of four build options.

### Visual Studio 2022 (recommended)

1. Open Visual Studio 2022.
2. Click "Open a local folder".
3. Select the repository root folder.
4. Visual Studio reads `CMakePresets.json` automatically.
5. Select a configuration preset from the toolbar dropdown, such as `windows-msvc-debug`.
6. Press F5 to build and run the application.

### Visual Studio Code

1. Install the C/C++ and CMake Tools extensions.
2. Open the repository root folder in VS Code.
3. Select a configure preset when prompted, such as `windows-msvc-debug` or `windows-ninja-debug`.
4. Press F7 to build.
5. Press F5 to run and debug.

### Automated PowerShell script

The repository provides a build helper script in `scripts/build-windows.ps1`.
The script detects your Visual Studio installation automatically.

Run the default debug build:
```powershell
.\scripts\build-windows.ps1
```

Build with a specific preset:
```powershell
.\scripts\build-windows.ps1 -Preset windows-msvc-debug
```

Clean the build directory before building:
```powershell
.\scripts\build-windows.ps1 -Preset windows-msvc-debug -Clean
```

### Manual CMake command line

Open a terminal with CMake and your C++ compiler in PATH.

Configure the build files:
```powershell
cmake -B build
```

Compile the binary:
```powershell
cmake --build build --config Debug
```

Start the application:
```powershell
.\build\Debug\geo_sandbox.exe
```

## Dependency management

CMake downloads all external dependencies automatically during configuration.
`FetchContent` pulls `raylib`, `imgui`, `rlimgui`, and `glm` directly from GitHub.
You do not need to install local libraries manually.

If you already use vcpkg, you can pass the toolchain file:
```powershell
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[path/to/vcpkg]/scripts/buildsystems/vcpkg.cmake
```

## Troubleshooting

### PowerShell script execution policy

PowerShell may block unsigned local scripts by default.
Allow local scripts for your user account:
```powershell
Set-ExecutionPolicy -ExecutionPolicy RemoteSigned -Scope CurrentUser
```

You can also bypass execution policy for one execution:
```powershell
pwsh -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
```

### Missing MSVC or Windows SDK

CMake requires both the MSVC C++ compiler and the Windows SDK.
Open the Visual Studio Installer and click Modify.
Verify that "Desktop development with C++" is selected.
Ensure "MSVC v143" and the latest "Windows 10/11 SDK" components are checked in the installation details.

### OpenGL driver requirements

Raylib creates an OpenGL 3.3 context on startup.
Default Windows display drivers or remote desktop sessions may only provide OpenGL 1.1.
Install current graphics drivers directly from your GPU vendor (NVIDIA, AMD, or Intel).
When running inside a virtual machine, enable 3D hardware acceleration in hypervisor settings.
