#pragma once

#include "core/Demo.h"
#include <raylib.h>

class TriangleDemo : public Demo {
public:
    void init() override;
    void destroy() override;
    void render() override;
};
