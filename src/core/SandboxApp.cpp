#include "core/SandboxApp.h"
#include <iostream>

#define RAYGUI_IMPLEMENTATION
#include "vendor/raygui.h"
#include "vendor/style_cyber.h"

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

    GuiLoadStyleCyber();

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

    renderUI();
    EndDrawing();
}

void SandboxApp::renderUI() {
    if (isUiCollapsed) {
        if (GuiButton(Rectangle{ 10, 10, 120, 24 }, "Demo Selector")) {
            isUiCollapsed = false;
        }
    } else {
        // Compact window box
        if (GuiWindowBox(Rectangle{ 10, 10, 300, 50 }, "Demo Selector")) {
            isUiCollapsed = true;
            isDropdownActive = false;
        }
        
        GuiLabel(Rectangle{ 15, 37, 45, 20 }, "Demo:");
        
        int active = currentDemoIndex;
        // Dropdown inline with the label
        if (GuiDropdownBox(Rectangle{ 60, 37, 230, 20 }, "Rainbow Triangle;Catmull-Clark;Tubular Lofting", &active, isDropdownActive)) {
            isDropdownActive = !isDropdownActive;
        }
        currentDemoIndex = active;
    }
}

void SandboxApp::cleanup() {
    triangleDemo.destroy();
    CloseWindow();
}
