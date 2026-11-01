#pragma once

#include "demos/Demo.h"
#include <memory>
#include <vector>

class SandboxApp {
public:
    void run();
    void registerDemo(std::unique_ptr<Demo> demo);

private:
    void initWindow();
    void mainLoop();
    void cleanup();

    void drawFrame();
    void renderUI();

    std::vector<std::unique_ptr<Demo>> demos;
    std::vector<const char*> demoNames;
    int currentDemoIndex = 0;
};
