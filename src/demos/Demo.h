#pragma once

class Demo {
public:
    virtual ~Demo() = default;

    virtual const char* name() const = 0;

    // Called once when the demo is loaded
    virtual void init() = 0;

    // Called when the demo becomes active
    virtual void enter() {}

    // Called every frame with elapsed time in seconds
    virtual void update(float /*dt*/) {}

    // Called every frame to render the demo
    virtual void render() = 0;

    // Called every frame to render the demo's UI
    virtual void renderUI() {}

    // Called when the demo transitions away
    virtual void exit() {}

    // Called when the demo is unloaded
    virtual void destroy() = 0;
};
