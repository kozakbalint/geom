#pragma once

class Demo {
public:
    virtual ~Demo() = default;
    
    // Called once when the demo is loaded
    virtual void init() = 0;
    
    // Called every frame to render the demo
    virtual void render() = 0;
    
    // Called when the demo is unloaded
    virtual void destroy() = 0;
};
