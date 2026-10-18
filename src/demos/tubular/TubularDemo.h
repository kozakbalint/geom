#pragma once

#include "demos/Demo.h"
#include "geometry/tubular/CircleSkinning.h"
#include "geometry/tubular/DiskBezier.h"
#include <raylib.h>
#include <vector>
#include <string>

enum class TubularAlgorithm {
    KUNKLI_HOFFMANN = 0,
    DISK_BEZIER = 1
};

enum class ViewMode {
    MODE_2D = 0,
    MODE_3D = 1
};

enum class InputState {
    IDLE,
    PLACING_RADIUS,
    DRAGGING_CENTER,
    DRAGGING_RADIUS
};

class TubularDemo : public Demo {
public:
    TubularDemo() = default;
    ~TubularDemo() override = default;

    void init() override;
    void render() override;
    void renderUI() override;
    void destroy() override;

private:
    void handleInput2D();
    void handleInput3D();
    void renderScene2D();
    void renderScene3D();

    void drawGrid2D();
    void resetCamera2D();
    void resetCamera3D();

    void loadPreset(int presetIndex);
    void recomputeSkin();

    // 2D Coordinate conversions (Y is UP)
    Vector2 worldToScreen2D(const Vector2& world) const;
    Vector2 screenToWorld2D(const Vector2& screen) const;

    // 2D Drawing helpers
    void drawSkinCurves2D();
    void drawContactPoints2D();
    void drawTangents2D();
    void drawRadicalElements2D();
    void drawFilledSkinRibbon2D();

    // 3D Drawing helpers
    void draw3DSurfaceMesh();
    void draw3DSpheres();
    void draw3DTouchingCircles();
    void draw3DLongitudinalRibs();
    void drawControlPolygon2D();
    void drawControlPolygon3D();

    // Style presets
    void applyPaperShadedStyle();
    void applyPaperStructuralStyle();

    // View mode: 2D Canvas vs 3D Tubular Surface
    ViewMode currentMode{ ViewMode::MODE_2D };

    // Cameras
    Camera2D camera2D{ {0.0f, 0.0f}, {0.0f, 0.0f}, 0.0f, 1.0f };
    Camera3D camera3D{ { 0.0f, 0.0f, 6.2f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, 45.0f, CAMERA_PERSPECTIVE };

    // Spheres data (used in both 2D and 3D)
    std::vector<core::SphereData3D> spheres;

    // Skinning results cache
    core::SkinningResult currentSkin2D;
    core::SphereSkinningResult3D currentSkin3D;

    // Algorithm selection & Disk Bézier state
    TubularAlgorithm currentAlgorithm{ TubularAlgorithm::KUNKLI_HOFFMANN };
    core::BezierDegreeMode bezierDegreeMode{ core::BezierDegreeMode::PIECEWISE_CUBIC_SPLINE };
    bool showControlPolygon{ true };
    core::DiskBezierResult2D currentDisk2D;
    core::DiskBezierResult3D currentDisk3D;

    // 2D Interactive placement / editing state
    InputState inputState{ InputState::IDLE };
    Vector2 pendingCenter2D{ 0.0f, 0.0f };
    float pendingRadius2D{ 0.0f };
    int selectedSphereIndex{ -1 };
    int hoveredSphereIndex{ -1 };
    bool isHoveringRadiusHandle{ false };

    // 2D Display options
    bool showGrid2D{ true };
    bool showCircles2D{ true };
    bool showLabels2D{ true };
    bool showSkinCurves2D{ true };
    bool showContactPoints2D{ true };
    bool showTangents2D{ true };
    bool showRadicalLines2D{ false };
    bool showRadicalCenters2D{ false };
    bool showFilledSkin2D{ true };

    // 3D Display options (default to clean paper surface view)
    bool showSpheres3D{ false };
    bool showTouchingCircles3D{ false };
    bool showLongitudinalRibs3D{ false };
    bool showWireframe3D{ false };
    bool showSurface3D{ true };
    bool showGrid3D{ true };

    // Algorithm parameters
    float tangentScale{ 2.0f };
    int curveResolution2D{ 48 };
    int phiResolution3D{ 36 };
    int tResolution3D{ 24 };

    // 3D Lighting & Blinn-Phong material (matching glossy paper Figures 15/16)
    Vector3 lightDir{ 0.7f, 1.2f, 1.0f };
    float lightIntensity{ 1.0f };
    float surfaceColor[3]{ 48.0f / 255.0f, 54.0f / 255.0f, 138.0f / 255.0f }; // Iconic Kunkli-Hoffmann Indigo
    float shininess{ 32.0f };
    float specularStrength{ 0.55f };

    // Status message
    std::string statusMessage{ "2D Canvas: Click to place circle center. Switch to 3D View anytime." };
};
