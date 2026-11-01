#include "app/SandboxApp.h"
#include <iostream>

#include <raylib.h>
#include "demos/subdivision/SubdivisionDemo.h"
#include "demos/tubular/TubularDemo.h"
#include "imgui.h"
#include "rlImGui.h"
#include "utils/PropertyGrid.h"

const int WIDTH = 800;
const int HEIGHT = 600;

void SandboxApp::run() {
    initWindow();
    mainLoop();
    cleanup();
}

void SandboxApp::registerDemo(std::unique_ptr<Demo> demo) {
    if (!demo) return;
    demoNames.push_back(demo->name());
    demos.push_back(std::move(demo));
}

void SandboxApp::initWindow() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WIDTH, HEIGHT, "Geo Sandbox");
    SetExitKey(KEY_NULL); // Prevent ESC key from closing the window

    rlImGuiSetup(true);

    if (demos.empty()) {
        registerDemo(std::make_unique<SubdivisionDemo>());
        registerDemo(std::make_unique<TubularDemo>());
    }

    for (auto& demo : demos) {
        demo->init();
    }

    if (!demos.empty() && currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
        demos[currentDemoIndex]->enter();
    }
}

void SandboxApp::mainLoop() {
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
            demos[currentDemoIndex]->update(dt);
        }
        drawFrame();
    }
}

void SandboxApp::drawFrame() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    if (currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
        demos[currentDemoIndex]->render();
    }

    rlImGuiBegin();
    renderUI();
    rlImGuiEnd();

    EndDrawing();
}

void SandboxApp::renderUI() {
    ImGui::SetNextWindowPos(ImVec2(10.0f, 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(320.0f, 600.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300.0f, 200.0f), ImVec2(FLT_MAX, FLT_MAX));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoSavedSettings;
    
    ImGui::Begin("Settings", nullptr, flags);
    
    if (ImGui::CollapsingHeader("Demo Selector", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (auto grid = ui::PropertyGrid::begin("DemoSelectorGrid")) {
            int prevIndex = currentDemoIndex;
            grid.combo("Demo", &currentDemoIndex, demoNames.data(), static_cast<int>(demoNames.size()));
            if (currentDemoIndex != prevIndex) {
                if (prevIndex >= 0 && prevIndex < static_cast<int>(demos.size())) {
                    demos[prevIndex]->exit();
                }
                if (currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
                    demos[currentDemoIndex]->enter();
                }
            }
        }
    }

    if (currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
        demos[currentDemoIndex]->renderUI();
    }

    ImGui::End();
}

void SandboxApp::cleanup() {
    if (currentDemoIndex >= 0 && currentDemoIndex < static_cast<int>(demos.size())) {
        demos[currentDemoIndex]->exit();
    }
    for (auto& demo : demos) {
        demo->destroy();
    }
    demos.clear();
    demoNames.clear();

    rlImGuiShutdown();
    CloseWindow();
}
