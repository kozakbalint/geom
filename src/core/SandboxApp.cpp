#include "core/SandboxApp.h"
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

    triangleDemo.init();
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
        triangleDemo.render();
    }

    rlImGuiBegin();
    renderUI();
    rlImGuiEnd();

    EndDrawing();
}

void SandboxApp::renderUI() {
    ImGui::SetNextWindowSize(ImVec2(240, 120), ImGuiCond_FirstUseEver);
    ImGui::Begin("Demo Selector");
    
    ImGui::PushItemWidth(-1);
    const char* items[] = { "Rainbow Triangle", "Catmull-Clark", "Tubular Lofting" };
    ImGui::Combo("##Demo", &currentDemoIndex, items, IM_ARRAYSIZE(items));
    ImGui::PopItemWidth();
    
    ImGui::End();
}

void SandboxApp::cleanup() {
    triangleDemo.destroy();
    rlImGuiShutdown();
    CloseWindow();
}
