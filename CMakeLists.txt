cmake_minimum_required(VERSION 3.20)
project(GeoSandbox VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Fetch Raylib (bypassing vcpkg to fix AMD bug)
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CUSTOMIZE_BUILD ON CACHE BOOL "" FORCE)
FetchContent_Declare(
  raylib
  GIT_REPOSITORY https://github.com/raysan5/raylib.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(raylib)

add_executable(geo_sandbox WIN32
    src/core/main.cpp
    src/core/SandboxApp.cpp
    src/demos/TriangleDemo.cpp
)

target_include_directories(geo_sandbox PRIVATE src)

target_link_libraries(geo_sandbox PRIVATE
    raylib
)
set_target_properties(geo_sandbox PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
