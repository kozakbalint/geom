#include "app/SandboxApp.h"
#include <iostream>

#include "imgui.h"
#include "rlImGui.h"

const int WIDTH = 800;
const int HEIGHT = 600;

void SandboxApp::run() {
    initWindow();
    mainLoop();
    cleanup();
}

void SandboxApp::initWindow() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(WIDTH, HEIGHT, "Geo Sandbox");

    rlImGuiSetup(true);

    subdivisionDemo.init();
    tubularDemo.init();
}

void SandboxApp::mainLoop() {
    while (!WindowShouldClose()) {
        drawFrame();
    }
}

void SandboxApp::drawFrame() {
    BeginDrawing();
    ClearBackground(DARKGRAY);

    if (currentDemoIndex == 0) {
        subdivisionDemo.render();
    } else if (currentDemoIndex == 1) {
        tubularDemo.render();
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
        ImGui::PushItemWidth(-1);
        const char* items[] = { "Subdivision", "Tubular Shapes" };
        ImGui::Combo("##Demo", &currentDemoIndex, items, IM_ARRAYSIZE(items));
        ImGui::PopItemWidth();
    }

    if (currentDemoIndex == 0) {
        subdivisionDemo.renderUI();
    } else if (currentDemoIndex == 1) {
        tubularDemo.renderUI();
    }

    ImGui::End();
}

void SandboxApp::cleanup() {
    subdivisionDemo.destroy();
    tubularDemo.destroy();
    rlImGuiShutdown();
    CloseWindow();
}
