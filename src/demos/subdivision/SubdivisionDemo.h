#pragma once

#include "demos/Demo.h"
#include "geometry/mesh/HalfEdgeConversion.h"
#include <raylib.h>
#include <string>

class SubdivisionDemo : public Demo {
public:
    void init() override;
    void render() override;
    void renderUI() override;
    void destroy() override;

private:
    void rebuildCache();
    void normalizeMesh(Mesh& mesh);
    bool loadFile(const std::string& path);

    enum Schema {
        SCHEMA_LOOP = 0,
        SCHEMA_BUTTERFLY,
        SCHEMA_CATMULL_CLARK
    };

    core::SubdividedMeshCache cache;
    Mesh currentBaseMesh = {0};
    Model loadedModel = {0};
    bool isModelLoaded = false;
    bool meshLoaded = false;
    std::string loadedFileName = "";
    std::string statusMessage = "";
    int loadedVertexCount = 0;
    int loadedTriangleCount = 0;
    Material material;
    Material wireMaterial;

    int currentSchema = SCHEMA_LOOP;
    int currentIteration = 0;
    int maxIterations = 4;
    bool wireframe = true;
    float faceOpacity = 1.0f;
    
    Camera3D camera = {0};
    
    Shader flatShader;
    int lightDirLoc;
    int lightIntensityLoc;
    Vector3 lightDir = { 1.0f, 1.0f, 1.0f };
    float lightYaw = 45.0f;
    float lightPitch = 35.0f;
    float lightIntensity = 1.0f;
    float diffuseColorF[3] = { 200.0f / 255.0f, 200.0f / 255.0f, 200.0f / 255.0f };
    Color diffuseColor = { 200, 200, 200, 255 };
};
