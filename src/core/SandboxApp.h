#pragma once

#include "demos/TriangleDemo.h"
#include <raylib.h>

class SandboxApp {
public:
    void run();

private:
    void initWindow();
    void mainLoop();
    void cleanup();

    void drawFrame();
    void renderUI();

    int currentDemoIndex = 0;
    TriangleDemo triangleDemo;
};
