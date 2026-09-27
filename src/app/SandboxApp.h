#pragma once

#include "demos/subdivision/SubdivisionDemo.h"
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
    SubdivisionDemo subdivisionDemo;
};
