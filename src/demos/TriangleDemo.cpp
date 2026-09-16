#include "demos/TriangleDemo.h"
#include <rlgl.h>

void TriangleDemo::init() {
}

void TriangleDemo::destroy() {
}

void TriangleDemo::render() {
    rlBegin(RL_TRIANGLES);
        // Top vertex (Red)
        rlColor3f(1.0f, 0.0f, 0.0f);
        rlVertex2f(400.0f, 100.0f);
        
        // Bottom-left vertex (Green)
        rlColor3f(0.0f, 1.0f, 0.0f);
        rlVertex2f(200.0f, 500.0f);
        
        // Bottom-right vertex (Blue)
        rlColor3f(0.0f, 0.0f, 1.0f);
        rlVertex2f(600.0f, 500.0f);
    rlEnd();
}
