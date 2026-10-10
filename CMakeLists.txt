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
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(raylib)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        docking
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)

FetchContent_Declare(
  rlimgui
  GIT_REPOSITORY https://github.com/raylib-extras/rlImGui.git
  GIT_TAG        main
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(rlimgui)

add_executable(geo_sandbox WIN32
    src/app/main.cpp
    src/app/SandboxApp.cpp
    src/utils/FileDialog.cpp
    src/utils/STLLoader.cpp
    src/geometry/mesh/HalfEdgeConversion.cpp
    src/geometry/subdivision/LoopSubdivision.cpp
    src/geometry/subdivision/ButterflySubdivision.cpp
    src/geometry/subdivision/CatmullClarkSubdivision.cpp
    src/demos/subdivision/SubdivisionDemo.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${rlimgui_SOURCE_DIR}/rlImGui.cpp
)

target_include_directories(geo_sandbox PRIVATE 
    src
    ${imgui_SOURCE_DIR}
    ${rlimgui_SOURCE_DIR}
)

find_package(glm CONFIG QUIET)
if(NOT TARGET glm::glm)
  FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(glm)
endif()

target_link_libraries(geo_sandbox PRIVATE
    raylib
    glm::glm
)

if(WIN32)
    target_link_libraries(geo_sandbox PRIVATE comdlg32)
endif()

if(MSVC)
  set_target_properties(geo_sandbox PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()
