#include "demos/tubular/TubularDemo.h"
#include "utils/PropertyGrid.h"
#include <imgui.h>
#include <raymath.h>
#include <rlgl.h>
#include <algorithm>
#include <cmath>

void TubularDemo::init() {
    resetCamera2D();
    resetCamera3D();
    spheres.clear();
    inputState = InputState::IDLE;
    selectedSphereIndex = -1;
    hoveredSphereIndex = -1;
    isHoveringRadiusHandle = false;

    // Load initial S-curve preset
    loadPreset(0);
}

void TubularDemo::resetCamera2D() {
    camera2D.target = { 0.0f, 0.0f };
    camera2D.offset = { (float)GetScreenWidth() * 0.5f, (float)GetScreenHeight() * 0.5f };
    camera2D.rotation = 0.0f;
    camera2D.zoom = 120.0f; // 1 unit = 120 pixels
}

void TubularDemo::resetCamera3D() {
    // Aligned directly with front view so 3D matches 2D orientation seamlessly
    camera3D.position = { 0.0f, 0.0f, 6.5f };
    camera3D.target = { 0.0f, 0.0f, 0.0f };
    camera3D.up = { 0.0f, 1.0f, 0.0f };
    camera3D.fovy = 45.0f;
    camera3D.projection = CAMERA_PERSPECTIVE;
}

void TubularDemo::destroy() {
    spheres.clear();
}

Vector2 TubularDemo::worldToScreen2D(const Vector2& world) const {
    return {
        camera2D.offset.x + (world.x - camera2D.target.x) * camera2D.zoom,
        camera2D.offset.y - (world.y - camera2D.target.y) * camera2D.zoom
    };
}

Vector2 TubularDemo::screenToWorld2D(const Vector2& screen) const {
    return {
        camera2D.target.x + (screen.x - camera2D.offset.x) / camera2D.zoom,
        camera2D.target.y - (screen.y - camera2D.offset.y) / camera2D.zoom
    };
}

void TubularDemo::recomputeSkin() {
    if (spheres.size() < 2) {
        currentSkin2D.isValid = false;
        currentSkin3D.isValid = false;
        currentSkin2D.errorMessage = "Add at least 2 spheres/circles.";
        currentSkin3D.errorMessage = "Add at least 2 spheres/circles.";
        return;
    }

    // 2D Skinning (using X, Y in standard Cartesian coords, Y is UP)
    std::vector<core::CircleData> circles2D;
    circles2D.reserve(spheres.size());
    for (const auto& s : spheres) {
        circles2D.push_back({ { s.center.x, s.center.y }, s.radius });
    }
    if (currentAlgorithm == TubularAlgorithm::KUNKLI_HOFFMANN) {
        currentSkin2D = core::CircleSkinning::computeSkinning(circles2D, tangentScale, curveResolution2D);
        currentSkin3D = core::CircleSkinning::computeSkinning3D(spheres, tangentScale, phiResolution3D, tResolution3D);
    } else {
        core::DiskBezierConfig2D cfg2D;
        cfg2D.mode = bezierDegreeMode;
        cfg2D.samplesPerSegment = curveResolution2D;
        cfg2D.endCapStyle = core::EndCapStyle::ROUND;
        currentDisk2D = core::DiskBezier::computeSkinning2D(circles2D, cfg2D);
        currentSkin2D = currentDisk2D.skin;

        core::DiskBezierConfig3D cfg3D;
        cfg3D.mode = bezierDegreeMode;
        cfg3D.tSamplesPerSegment = tResolution3D;
        cfg3D.phiResolution = phiResolution3D;
        cfg3D.endCapStyle = core::EndCapStyle::OPEN;
        currentDisk3D = core::DiskBezier::computeSkinning3D(spheres, cfg3D);
        currentSkin3D = currentDisk3D.skin;
    }

    if (!currentSkin2D.isValid && currentMode == ViewMode::MODE_2D) {
        statusMessage = "2D Skinning notice: " + currentSkin2D.errorMessage;
    } else if (!currentSkin3D.isValid && currentMode == ViewMode::MODE_3D) {
        statusMessage = "3D Skinning notice: " + currentSkin3D.errorMessage;
    }
}

void TubularDemo::loadPreset(int presetIndex) {
    spheres.clear();
    selectedSphereIndex = -1;
    inputState = InputState::IDLE;

    if (presetIndex == 0) {
        // S-Curve Pipeline (Fig. 9 / Fig. 14)
        spheres.push_back({ { -2.0f, -0.6f,  0.0f }, 0.45f });
        spheres.push_back({ { -0.7f,  0.6f,  0.0f }, 0.35f });
        spheres.push_back({ {  0.7f, -0.3f,  0.0f }, 0.50f });
        spheres.push_back({ {  2.0f,  0.7f,  0.0f }, 0.40f });
        statusMessage = "Preset 1 loaded: S-curve pipeline (Fig. 9/14).";
    } else if (presetIndex == 1) {
        // Varying Radii (Paper Fig. 1 style)
        spheres.push_back({ { -2.4f, -0.4f,  0.0f }, 0.55f });
        spheres.push_back({ { -1.2f,  0.4f,  0.0f }, 0.25f });
        spheres.push_back({ {  0.0f,  0.7f,  0.0f }, 0.60f });
        spheres.push_back({ {  1.3f,  0.1f,  0.0f }, 0.30f });
        spheres.push_back({ {  2.4f, -0.6f,  0.0f }, 0.50f });
        statusMessage = "Preset 2 loaded: Varying radii sequence (Fig. 1).";
    } else if (presetIndex == 2) {
        // Figure 15: 3D Non-coplanar Knotted Tube (Paper Fig. 15)
        // 20 spheres perfectly reproducing the space knot with open cylinder rims!
        spheres = {
            { { -0.594f,  1.512f,  0.400f }, 0.288f }, // 0: Top chimney open rim
            { { -0.666f,  0.828f,  0.200f }, 0.216f }, // 1: Upper neck
            { { -0.180f,  0.810f, -0.400f }, 0.180f }, // 2: Turning right/back
            { {  0.720f,  1.044f, -0.700f }, 0.270f }, // 3: Upper right arch peak
            { {  1.674f,  0.828f, -0.600f }, 0.234f }, // 4: Upper right outer curve
            { {  1.998f,  0.144f, -0.300f }, 0.198f }, // 5: Right side descent
            { {  1.818f, -0.504f,  0.000f }, 0.180f }, // 6: Lower right curve
            { {  1.206f, -1.080f,  0.300f }, 0.180f }, // 7: Bottom right sweep
            { {  0.342f, -0.864f,  0.600f }, 0.234f }, // 8: Sweep towards bladder
            { { -0.540f, -1.080f,  0.800f }, 0.360f }, // 9: Entering large bladder
            { { -1.386f, -1.224f,  0.800f }, 0.504f }, // 10: Huge bulb (largest sphere)
            { { -1.818f, -0.612f,  0.500f }, 0.234f }, // 11: Left turnaround
            { { -1.314f, -0.036f,  0.200f }, 0.198f }, // 12: Left ascent
            { { -0.846f,  0.216f,  0.400f }, 0.252f }, // 13: Mid bridge ascending
            { { -0.162f,  0.396f,  0.700f }, 0.360f }, // 14: Center overpass (front)
            { {  0.486f,  0.180f,  0.500f }, 0.252f }, // 15: Overpass descending right
            { {  0.954f, -0.288f,  0.200f }, 0.198f }, // 16: Looping down
            { {  1.098f, -0.828f, -0.100f }, 0.180f }, // 17: Bottom hook
            { {  0.810f, -1.080f, -0.100f }, 0.162f }, // 18: Hook curling left
            { {  0.486f, -0.936f,  0.000f }, 0.144f }  // 19: End 2 terminating rim
        };
        currentMode = ViewMode::MODE_3D;
        applyPaperShadedStyle();
        camera3D.position = { 0.0f, 0.0f, 6.2f };
        camera3D.target = { 0.0f, 0.0f, 0.0f };
        statusMessage = "Preset 3 loaded: Figure 15 (3D non-coplanar space tube/knot with open cylinder ends).";
    } else if (presetIndex == 3) {
        // Figure 16 row 3: 3D Anatomical Seahorse (Paper Fig. 16)
        // 21 spheres reconstructing the authentic anatomy (snout, crown, throat, belly, spiral tail)
        spheres = {
            // Snout / Beak
            { {  0.924f,  1.224f,  0.000f }, 0.054f }, // 0: Snout tip
            { {  0.684f,  1.392f,  0.000f }, 0.096f }, // 1: Snout base
            { {  0.468f,  1.584f,  0.000f }, 0.216f }, // 2: Forehead
            // Head & Crown
            { {  0.180f,  1.848f,  0.000f }, 0.324f }, // 3: Head top crest
            { { -0.132f,  1.872f,  0.000f }, 0.324f }, // 4: Upper nape
            { { -0.372f,  1.584f,  0.000f }, 0.240f }, // 5: Nape
            // Neck & Throat
            { { -0.444f,  1.248f,  0.000f }, 0.192f }, // 6: Neck indent / throat pinch
            { { -0.348f,  0.816f,  0.000f }, 0.300f }, // 7: Throat / upper chest
            // Bulging Belly & Stomach
            { { -0.156f,  0.384f,  0.000f }, 0.456f }, // 8: Chest
            { { -0.012f, -0.144f,  0.000f }, 0.480f }, // 9: Mid belly (maximum bulge)
            { { -0.084f, -0.648f,  0.000f }, 0.336f }, // 10: Lower belly
            // Curled Spiral Tail
            { { -0.204f, -1.080f,  0.000f }, 0.216f }, // 11: Upper tail
            { { -0.252f, -1.416f,  0.000f }, 0.168f }, // 12: Tail
            { { -0.252f, -1.704f,  0.000f }, 0.144f }, // 13: Tail
            { { -0.084f, -1.944f,  0.000f }, 0.132f }, // 14: Bottom outer curve
            { {  0.180f, -2.016f,  0.000f }, 0.132f }, // 15: Bottom curve
            { {  0.468f, -1.896f,  0.000f }, 0.120f }, // 16: Curving right/up
            { {  0.612f, -1.608f,  0.000f }, 0.108f }, // 17: Curving up
            { {  0.516f, -1.320f,  0.000f }, 0.096f }, // 18: Curving left
            { {  0.300f, -1.176f,  0.000f }, 0.084f }, // 19: Curving down/left
            { {  0.132f, -1.272f,  0.000f }, 0.066f }  // 20: Inner spiral tip
        };
        currentMode = ViewMode::MODE_3D;
        applyPaperShadedStyle();
        camera3D.position = { 0.0f, 0.0f, 6.2f };
        camera3D.target = { 0.0f, 0.0f, 0.0f };
        statusMessage = "Preset 4 loaded: Figure 16 (3D Seahorse with head, throat, belly, and curled spiral tail).";
    }

    recomputeSkin();
}

void TubularDemo::handleInput2D() {
    Vector2 screenMouse = GetMousePosition();
    Vector2 worldMouse = screenToWorld2D(screenMouse);

    // Cancel circle placement on right mouse button click
    if (inputState == InputState::PLACING_RADIUS) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
            inputState = InputState::IDLE;
            statusMessage = "Circle placement cancelled.";
            return;
        }
    }

    // Pan camera with Right Mouse Button or Middle Mouse Button (only when not placing circle)
    if (inputState != InputState::PLACING_RADIUS) {
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
            Vector2 delta = GetMouseDelta();
            camera2D.target.x -= delta.x / camera2D.zoom;
            camera2D.target.y += delta.y / camera2D.zoom; // Y is UP
        }
    }

    // Zoom camera with Mouse Wheel (anchored around cursor)
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector2 mouseBefore = screenToWorld2D(screenMouse);
        camera2D.zoom = std::clamp(camera2D.zoom * (1.0f + wheel * 0.12f), 20.0f, 600.0f);
        Vector2 mouseAfter = screenToWorld2D(screenMouse);
        camera2D.target.x += mouseBefore.x - mouseAfter.x;
        camera2D.target.y += mouseBefore.y - mouseAfter.y;
    }

    // Hover detection over circles
    hoveredSphereIndex = -1;
    isHoveringRadiusHandle = false;

    if (inputState == InputState::IDLE) {
        for (int i = (int)spheres.size() - 1; i >= 0; --i) {
            Vector2 c2D = { spheres[i].center.x, spheres[i].center.y };
            float d = Vector2Distance(worldMouse, c2D);

            // Handle on right perimeter
            Vector2 handlePos = { c2D.x + spheres[i].radius, c2D.y };
            if (Vector2Distance(worldMouse, handlePos) <= 12.0f / camera2D.zoom) {
                hoveredSphereIndex = i;
                isHoveringRadiusHandle = true;
                break;
            }
            if (d <= spheres[i].radius) {
                hoveredSphereIndex = i;
                break;
            }
        }
    }

    // Left click handling
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        if (inputState == InputState::IDLE) {
            if (hoveredSphereIndex != -1) {
                selectedSphereIndex = hoveredSphereIndex;
                if (isHoveringRadiusHandle) {
                    inputState = InputState::DRAGGING_RADIUS;
                    statusMessage = "Dragging radius handle. Release mouse to finish.";
                } else {
                    inputState = InputState::DRAGGING_CENTER;
                    statusMessage = "Dragging center. Release mouse to finish.";
                }
            } else {
                // Start placing a new circle
                pendingCenter2D = worldMouse;
                pendingRadius2D = 0.35f;
                inputState = InputState::PLACING_RADIUS;
                statusMessage = "Move mouse to set radius, left-click to place, right-click to cancel.";
            }
        } else if (inputState == InputState::PLACING_RADIUS) {
            // Confirm placement of new circle
            float r = Vector2Distance(worldMouse, pendingCenter2D);
            if (r > 0.05f) {
                spheres.push_back({ { pendingCenter2D.x, pendingCenter2D.y, 0.0f }, r });
                selectedSphereIndex = (int)spheres.size() - 1;
                recomputeSkin();
                statusMessage = "Circle placed! Click canvas to add next circle.";
            } else {
                statusMessage = "Radius too small, placement cancelled.";
            }
            inputState = InputState::IDLE;
        }
    }

    // Interactive radius sizing
    if (inputState == InputState::PLACING_RADIUS) {
        pendingRadius2D = std::max(0.05f, Vector2Distance(worldMouse, pendingCenter2D));
    }

    // Dragging center
    if (inputState == InputState::DRAGGING_CENTER) {
        if (selectedSphereIndex >= 0 && selectedSphereIndex < (int)spheres.size()) {
            Vector2 delta = GetMouseDelta();
            spheres[selectedSphereIndex].center.x += delta.x / camera2D.zoom;
            spheres[selectedSphereIndex].center.y -= delta.y / camera2D.zoom; // Y is UP
            recomputeSkin();
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            inputState = InputState::IDLE;
            statusMessage = "Circle moved.";
        }
    }

    // Dragging radius
    if (inputState == InputState::DRAGGING_RADIUS) {
        if (selectedSphereIndex >= 0 && selectedSphereIndex < (int)spheres.size()) {
            Vector2 c2D = { spheres[selectedSphereIndex].center.x, spheres[selectedSphereIndex].center.y };
            float newR = Vector2Distance(worldMouse, c2D);
            spheres[selectedSphereIndex].radius = std::max(0.05f, newR);
            recomputeSkin();
        }
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            inputState = InputState::IDLE;
            statusMessage = "Circle radius updated.";
        }
    }

    // Delete key
    if (IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE)) {
        if (selectedSphereIndex >= 0 && selectedSphereIndex < (int)spheres.size()) {
            spheres.erase(spheres.begin() + selectedSphereIndex);
            selectedSphereIndex = -1;
            recomputeSkin();
            statusMessage = "Selected circle deleted.";
        }
    }
}

void TubularDemo::handleInput3D() {
    Vector2 mouseDelta = GetMouseDelta();

    // Orbit 3D camera with Left Mouse Button
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        float rotSpeed = 0.005f;
        Vector3 view = Vector3Subtract(camera3D.position, camera3D.target);

        // Yaw
        Matrix yawMatrix = MatrixRotate(camera3D.up, -mouseDelta.x * rotSpeed);
        view = Vector3Transform(view, yawMatrix);

        // Pitch
        Vector3 right = Vector3Normalize(Vector3CrossProduct(camera3D.up, view));
        Matrix pitchMatrix = MatrixRotate(right, -mouseDelta.y * rotSpeed);
        Vector3 newView = Vector3Transform(view, pitchMatrix);

        float dot = Vector3DotProduct(Vector3Normalize(newView), camera3D.up);
        if (dot < 0.99f && dot > -0.99f) {
            view = newView;
        }

        camera3D.position = Vector3Add(camera3D.target, view);
    }

    // Pan 3D camera with Right Mouse Button or Middle Mouse Button
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT) || IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector3 view = Vector3Subtract(camera3D.position, camera3D.target);
        Vector3 right = Vector3Normalize(Vector3CrossProduct(camera3D.up, view));
        Vector3 up = Vector3Normalize(Vector3CrossProduct(view, right));

        float panSpeed = 0.005f * Vector3Length(view);
        Vector3 move = Vector3Add(Vector3Scale(right, -mouseDelta.x * panSpeed), Vector3Scale(up, mouseDelta.y * panSpeed));
        camera3D.target = Vector3Add(camera3D.target, move);
        camera3D.position = Vector3Add(camera3D.position, move);
    }

    // Zoom 3D camera with Mouse Wheel
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        Vector3 view = Vector3Subtract(camera3D.position, camera3D.target);
        float len = Vector3Length(view) * (1.0f - wheel * 0.1f);
        len = std::clamp(len, 0.5f, 50.0f);
        camera3D.position = Vector3Add(camera3D.target, Vector3Scale(Vector3Normalize(view), len));
    }
}

void TubularDemo::drawGrid2D() {
    float spacing = 0.5f;
    Color gridColor = { 38, 45, 58, 255 };

    float startX = floorf((camera2D.target.x - 12.0f) / spacing) * spacing;
    float endX = startX + 24.0f;
    float startY = floorf((camera2D.target.y - 12.0f) / spacing) * spacing;
    float endY = startY + 24.0f;

    for (float x = startX; x <= endX; x += spacing) {
        Vector2 p1 = worldToScreen2D({ x, startY });
        Vector2 p2 = worldToScreen2D({ x, endY });
        DrawLineV(p1, p2, gridColor);
    }
    for (float y = startY; y <= endY; y += spacing) {
        Vector2 p1 = worldToScreen2D({ startX, y });
        Vector2 p2 = worldToScreen2D({ endX, y });
        DrawLineV(p1, p2, gridColor);
    }

    // Origin crosshair (Cartesian axes)
    Vector2 oX1 = worldToScreen2D({ -0.4f, 0.0f });
    Vector2 oX2 = worldToScreen2D({  0.4f, 0.0f });
    Vector2 oY1 = worldToScreen2D({ 0.0f, -0.4f });
    Vector2 oY2 = worldToScreen2D({ 0.0f,  0.4f });
    DrawLineV(oX1, oX2, { 85, 100, 120, 255 });
    DrawLineV(oY1, oY2, { 85, 100, 120, 255 });
}

void TubularDemo::drawFilledSkinRibbon2D() {
    if (!currentSkin2D.isValid || currentSkin2D.segments.empty()) return;

    Color ribbonColor = { 41, 128, 185, 50 };

    for (const auto& seg : currentSkin2D.segments) {
        size_t count = std::min(seg.leftCurvePoints.size(), seg.rightCurvePoints.size());
        if (count < 2) continue;

        for (size_t k = 0; k < count - 1; ++k) {
            Vector2 l1 = worldToScreen2D(seg.leftCurvePoints[k]);
            Vector2 l2 = worldToScreen2D(seg.leftCurvePoints[k + 1]);
            Vector2 r1 = worldToScreen2D(seg.rightCurvePoints[k]);
            Vector2 r2 = worldToScreen2D(seg.rightCurvePoints[k + 1]);

            DrawTriangle(l1, r1, l2, ribbonColor);
            DrawTriangle(l2, r1, r2, ribbonColor);
        }
    }

    if (spheres.size() >= 2 && currentSkin2D.startCapPoints.size() >= 2) {
        Vector2 o0 = worldToScreen2D({ spheres[0].center.x, spheres[0].center.y });
        for (size_t k = 0; k < currentSkin2D.startCapPoints.size() - 1; ++k) {
            Vector2 p1 = worldToScreen2D(currentSkin2D.startCapPoints[k]);
            Vector2 p2 = worldToScreen2D(currentSkin2D.startCapPoints[k + 1]);
            DrawTriangle(o0, p1, p2, ribbonColor);
        }
    }

    if (spheres.size() >= 2 && currentSkin2D.endCapPoints.size() >= 2) {
        Vector2 on = worldToScreen2D({ spheres.back().center.x, spheres.back().center.y });
        for (size_t k = 0; k < currentSkin2D.endCapPoints.size() - 1; ++k) {
            Vector2 p1 = worldToScreen2D(currentSkin2D.endCapPoints[k]);
            Vector2 p2 = worldToScreen2D(currentSkin2D.endCapPoints[k + 1]);
            DrawTriangle(on, p1, p2, ribbonColor);
        }
    }
}

void TubularDemo::drawSkinCurves2D() {
    if (!currentSkin2D.isValid) return;

    Color leftColor = { 46, 204, 113, 255 };  // Emerald Green for left skin
    Color rightColor = { 230, 126, 34, 255 }; // Bright Orange for right skin
    Color capColor = { 189, 195, 199, 220 };
    float thick = 2.5f;

    for (const auto& seg : currentSkin2D.segments) {
        for (size_t k = 0; k + 1 < seg.leftCurvePoints.size(); ++k) {
            Vector2 p1 = worldToScreen2D(seg.leftCurvePoints[k]);
            Vector2 p2 = worldToScreen2D(seg.leftCurvePoints[k + 1]);
            DrawLineEx(p1, p2, thick, leftColor);
        }
        for (size_t k = 0; k + 1 < seg.rightCurvePoints.size(); ++k) {
            Vector2 p1 = worldToScreen2D(seg.rightCurvePoints[k]);
            Vector2 p2 = worldToScreen2D(seg.rightCurvePoints[k + 1]);
            DrawLineEx(p1, p2, thick, rightColor);
        }
    }

    for (size_t k = 0; k + 1 < currentSkin2D.startCapPoints.size(); ++k) {
        Vector2 p1 = worldToScreen2D(currentSkin2D.startCapPoints[k]);
        Vector2 p2 = worldToScreen2D(currentSkin2D.startCapPoints[k + 1]);
        DrawLineEx(p1, p2, thick, capColor);
    }
    for (size_t k = 0; k + 1 < currentSkin2D.endCapPoints.size(); ++k) {
        Vector2 p1 = worldToScreen2D(currentSkin2D.endCapPoints[k]);
        Vector2 p2 = worldToScreen2D(currentSkin2D.endCapPoints[k + 1]);
        DrawLineEx(p1, p2, thick, capColor);
    }
}

void TubularDemo::drawContactPoints2D() {
    if (!currentSkin2D.isValid) return;

    for (size_t i = 0; i < currentSkin2D.leftPoints.size(); ++i) {
        Vector2 pL = worldToScreen2D(currentSkin2D.leftPoints[i].point);
        DrawCircleV(pL, 5.0f, Color{ 46, 204, 113, 255 });
        DrawCircleLinesV(pL, 7.0f, Color{ 255, 255, 255, 200 });

        Vector2 pR = worldToScreen2D(currentSkin2D.rightPoints[i].point);
        DrawCircleV(pR, 5.0f, Color{ 230, 126, 34, 255 });
        DrawCircleLinesV(pR, 7.0f, Color{ 255, 255, 255, 200 });
    }
}

void TubularDemo::drawTangents2D() {
    if (!currentSkin2D.isValid) return;

    float thick = 2.0f;

    auto drawArrow = [&](const Vector2& startWorld, const Vector2& vecWorld, Color color) {
        if (Vector2Length(vecWorld) < 0.01f) return;
        Vector2 startScreen = worldToScreen2D(startWorld);
        Vector2 endScreen = worldToScreen2D(Vector2Add(startWorld, vecWorld));
        DrawLineEx(startScreen, endScreen, thick, color);

        Vector2 dir = Vector2Normalize(Vector2Subtract(endScreen, startScreen));
        Vector2 perp = { -dir.y, dir.x };
        float arrowSize = 8.0f;
        Vector2 arrowL = Vector2Subtract(endScreen, Vector2Add(Vector2Scale(dir, arrowSize), Vector2Scale(perp, arrowSize * 0.5f)));
        Vector2 arrowR = Vector2Subtract(endScreen, Vector2Subtract(Vector2Scale(dir, arrowSize), Vector2Scale(perp, arrowSize * 0.5f)));
        DrawTriangle(endScreen, arrowL, arrowR, color);
    };

    for (size_t i = 0; i < currentSkin2D.leftPoints.size(); ++i) {
        drawArrow(currentSkin2D.leftPoints[i].point, currentSkin2D.leftPoints[i].tangent, Color{ 39, 174, 96, 255 });
        drawArrow(currentSkin2D.rightPoints[i].point, currentSkin2D.rightPoints[i].tangent, Color{ 211, 84, 0, 255 });
    }
}

void TubularDemo::drawRadicalElements2D() {
    if (!currentSkin2D.isValid) return;

    if (showRadicalLines2D) {
        for (const auto& line : currentSkin2D.radicalLines) {
            Vector2 p1 = worldToScreen2D(Vector2Add(line.pointOnLine, Vector2Scale(line.dir, -4.0f)));
            Vector2 p2 = worldToScreen2D(Vector2Add(line.pointOnLine, Vector2Scale(line.dir, 4.0f)));
            DrawLineEx(p1, p2, 1.5f, Color{ 142, 68, 173, 160 });
            Vector2 pMid = worldToScreen2D(line.pointOnLine);
            DrawCircleV(pMid, 3.5f, Color{ 155, 89, 182, 255 });
        }
    }

    if (showRadicalCenters2D) {
        for (size_t i = 0; i < currentSkin2D.radicalCenters.size(); ++i) {
            Vector2 rc = worldToScreen2D(currentSkin2D.radicalCenters[i]);
            DrawCircleV(rc, 6.0f, Color{ 231, 76, 60, 255 });
            DrawCircleLinesV(rc, 8.0f, Color{ 255, 255, 255, 220 });
        }
    }
}

void TubularDemo::renderScene2D() {
    if (showGrid2D) {
        drawGrid2D();
    }

    // Spine connecting sphere centers
    if (spheres.size() >= 2) {
        for (size_t i = 0; i + 1 < spheres.size(); ++i) {
            Vector2 pA = worldToScreen2D({ spheres[i].center.x, spheres[i].center.y });
            Vector2 pB = worldToScreen2D({ spheres[i + 1].center.x, spheres[i + 1].center.y });
            DrawLineV(pA, pB, Color{ 100, 120, 145, 120 });
        }
    }

    // 1. Filled ribbon
    if (showFilledSkin2D) {
        drawFilledSkinRibbon2D();
    }

    // 2. Circles
    if (showCircles2D) {
        for (size_t i = 0; i < spheres.size(); ++i) {
            Vector2 c2D = { spheres[i].center.x, spheres[i].center.y };
            Vector2 screenCenter = worldToScreen2D(c2D);
            float screenRadius = spheres[i].radius * camera2D.zoom;
            bool isSelected = ((int)i == selectedSphereIndex);
            bool isHovered = ((int)i == hoveredSphereIndex);

            // Translucent fill
            Color fillColor = isSelected ? Color{ 52, 152, 219, 70 } : Color{ 255, 255, 255, 25 };
            DrawCircleV(screenCenter, screenRadius, fillColor);

            // Perimeter outline - using DrawCircleLinesV with float screenRadius (no phantom circles!)
            Color outlineColor = isSelected ? Color{ 52, 152, 219, 255 } : (isHovered ? Color{ 241, 196, 15, 255 } : Color{ 160, 175, 195, 180 });
            DrawCircleLinesV(screenCenter, screenRadius, outlineColor);

            // Center point
            DrawCircleV(screenCenter, 3.5f, isSelected ? Color{ 231, 76, 60, 255 } : Color{ 236, 240, 241, 255 });

            // Radius handle
            if (isSelected || isHovered) {
                Vector2 handlePos = worldToScreen2D({ c2D.x + spheres[i].radius, c2D.y });
                DrawCircleV(handlePos, 5.0f, Color{ 241, 196, 15, 255 });
                DrawLineV(screenCenter, handlePos, Color{ 241, 196, 15, 140 });
            }

            // Circle index label
            if (showLabels2D) {
                std::string label = "C" + std::to_string(i + 1);
                DrawText(label.c_str(), (int)screenCenter.x + 8, (int)screenCenter.y - 18, 16, Color{ 236, 240, 241, 220 });
            }
        }
    }

    // 3. Radical elements
    drawRadicalElements2D();

    // 4. Skin curves
    if (showSkinCurves2D) {
        drawSkinCurves2D();
    }
    // 5. Tangents
    if (showTangents2D) {
        drawTangents2D();
    }
    // 6. Contact points
    if (showContactPoints2D) {
        drawContactPoints2D();
    }

    // 7. Control polygon for Disk Bézier
    drawControlPolygon2D();

    // 8. Interactive preview during placement
    if (inputState == InputState::PLACING_RADIUS) {
        Vector2 screenCenter = worldToScreen2D(pendingCenter2D);
        float screenRadius = pendingRadius2D * camera2D.zoom;

        DrawCircleV(screenCenter, screenRadius, Color{ 46, 204, 113, 35 });
        DrawCircleLinesV(screenCenter, screenRadius, Color{ 46, 204, 113, 255 });
        DrawCircleV(screenCenter, 4.0f, Color{ 46, 204, 113, 255 });

        Vector2 screenMouse = GetMousePosition();
        DrawLineV(screenCenter, screenMouse, Color{ 46, 204, 113, 180 });
    }
}

void TubularDemo::drawControlPolygon2D() {
    if (currentAlgorithm != TubularAlgorithm::DISK_BEZIER || !showControlPolygon) return;
    if (currentDisk2D.controlPolygon.size() < 2) return;

    Color polyLineColor = { 180, 110, 220, 200 };
    Color polyPointColor = { 210, 140, 250, 255 };

    for (size_t i = 0; i + 1 < currentDisk2D.controlPolygon.size(); ++i) {
        Vector2 p1 = worldToScreen2D(currentDisk2D.controlPolygon[i]);
        Vector2 p2 = worldToScreen2D(currentDisk2D.controlPolygon[i + 1]);
        DrawLineEx(p1, p2, 1.8f, polyLineColor);
        DrawCircleV(p1, 3.5f, polyPointColor);
    }
    DrawCircleV(worldToScreen2D(currentDisk2D.controlPolygon.back()), 3.5f, polyPointColor);
}

void TubularDemo::drawControlPolygon3D() {
    if (currentAlgorithm != TubularAlgorithm::DISK_BEZIER || !showControlPolygon) return;
    if (currentDisk3D.controlPolygon.size() < 2) return;

    Color polyLineColor = { 180, 110, 220, 180 };
    Color polyPointColor = { 210, 140, 250, 230 };

    for (size_t i = 0; i + 1 < currentDisk3D.controlPolygon.size(); ++i) {
        DrawLine3D(currentDisk3D.controlPolygon[i], currentDisk3D.controlPolygon[i + 1], polyLineColor);
        DrawSphere(currentDisk3D.controlPolygon[i], 0.035f, polyPointColor);
    }
    DrawSphere(currentDisk3D.controlPolygon.back(), 0.035f, polyPointColor);
}

void TubularDemo::draw3DSpheres() {
    for (size_t i = 0; i < spheres.size(); ++i) {
        const auto& s = spheres[i];
        bool isSelected = ((int)i == selectedSphereIndex);

        Color wireColor = isSelected ? Color{ 52, 152, 219, 180 } : Color{ 140, 160, 180, 80 };
        DrawSphereWires(s.center, s.radius, 14, 14, wireColor);
        DrawSphere(s.center, 0.04f, isSelected ? Color{ 231, 76, 60, 255 } : Color{ 236, 240, 241, 200 });
    }
}

void TubularDemo::draw3DTouchingCircles() {
    if (!currentSkin3D.isValid) return;

    for (size_t i = 0; i < currentSkin3D.touchingCircles.size(); ++i) {
        const auto& tc = currentSkin3D.touchingCircles[i];
        const int steps = 36;
        Color ringColor = Color{ 46, 204, 113, 220 }; // Bright green for contact rings

        Vector3 prevPt = Vector3Add(tc.center, Vector3Add(tc.u, tc.v));
        for (int k = 0; k <= steps; ++k) {
            float phi = (float)k * 2.0f * PI / (float)steps;
            Vector3 pt = Vector3Add(tc.center, Vector3Add(Vector3Scale(tc.u, tc.radius * cosf(phi)), Vector3Scale(tc.v, tc.radius * sinf(phi))));
            if (k > 0) {
                DrawLine3D(prevPt, pt, ringColor);
            }
            prevPt = pt;
        }

        // Normal axis
        DrawLine3D(tc.center, Vector3Add(tc.center, Vector3Scale(tc.normal, 0.35f)), Color{ 241, 196, 15, 200 });
    }
}

void TubularDemo::applyPaperShadedStyle() {
    showSurface3D = true;
    showWireframe3D = false;
    showSpheres3D = false;
    showTouchingCircles3D = false;
    showLongitudinalRibs3D = false;
    surfaceColor[0] = 48.0f / 255.0f;
    surfaceColor[1] = 54.0f / 255.0f;
    surfaceColor[2] = 138.0f / 255.0f;
    shininess = 32.0f;
    specularStrength = 0.55f;
    statusMessage = "Applied Paper Shaded Style (Clean glossy indigo surface).";
}

void TubularDemo::applyPaperStructuralStyle() {
    showSurface3D = false;
    showWireframe3D = false;
    showSpheres3D = true;
    showTouchingCircles3D = true;
    showLongitudinalRibs3D = true;
    statusMessage = "Applied Paper Structural Style (Spheres, touching circles, and meridians).";
}

void TubularDemo::draw3DLongitudinalRibs() {
    if (!currentSkin3D.isValid || currentSkin3D.patches.empty()) return;

    Color ribColor = { 35, 75, 215, 230 }; // Rich blue matching paper wireframe diagrams
    for (const auto& patch : currentSkin3D.patches) {
        int M = patch.phiSteps;
        int K = patch.tSteps;
        int stepInc = std::max(1, M / 8); // 8 longitudinal ribs around circumference

        for (int stepPhi = 0; stepPhi < M; stepPhi += stepInc) {
            for (int stepT = 0; stepT < K; ++stepT) {
                const auto& v1 = patch.vertices[stepT * (M + 1) + stepPhi];
                const auto& v2 = patch.vertices[(stepT + 1) * (M + 1) + stepPhi];
                DrawLine3D(v1.position, v2.position, ribColor);
            }
        }
    }
}

void TubularDemo::draw3DSurfaceMesh() {
    if (!currentSkin3D.isValid || currentSkin3D.patches.empty()) return;

    Vector3 lDir = Vector3Normalize(lightDir);
    Vector3 lFill = Vector3Normalize(Vector3{ -0.6f, -0.4f, 0.5f });
    Vector3 camPos = camera3D.position;

    rlDisableBackfaceCulling();

    // 1. Solid shaded surface with Blinn-Phong specular highlighting (matching paper Figures 15/16)
    if (showSurface3D) {
        rlBegin(RL_TRIANGLES);

        for (const auto& patch : currentSkin3D.patches) {
            for (size_t idx = 0; idx + 2 < patch.indices.size(); idx += 3) {
                int i0 = patch.indices[idx];
                int i1 = patch.indices[idx + 1];
                int i2 = patch.indices[idx + 2];

                const auto& v0 = patch.vertices[i0];
                const auto& v1 = patch.vertices[i1];
                const auto& v2 = patch.vertices[i2];

                auto emitVertex = [&](const core::SurfaceVertex3D& v) {
                    Vector3 viewDir = Vector3Normalize(Vector3Subtract(camPos, v.position));
                    Vector3 halfDir = Vector3Normalize(Vector3Add(lDir, viewDir));

                    float nDotL = std::max(0.0f, Vector3DotProduct(v.normal, lDir));
                    float nDotFill = std::max(0.0f, Vector3DotProduct(v.normal, lFill)) * 0.20f;
                    float nDotH = std::max(0.0f, Vector3DotProduct(v.normal, halfDir));
                    float spec = (nDotL > 0.0f) ? powf(nDotH, shininess) * specularStrength * lightIntensity : 0.0f;

                    float diffuse = std::clamp(0.22f + 0.70f * nDotL * lightIntensity + nDotFill, 0.0f, 1.0f);

                    float r = std::clamp(surfaceColor[0] * diffuse + spec, 0.0f, 1.0f);
                    float g = std::clamp(surfaceColor[1] * diffuse + spec, 0.0f, 1.0f);
                    float b = std::clamp(surfaceColor[2] * diffuse + spec, 0.0f, 1.0f);

                    Color c = {
                        (unsigned char)(r * 255.0f),
                        (unsigned char)(g * 255.0f),
                        (unsigned char)(b * 255.0f),
                        255
                    };
                    rlNormal3f(v.normal.x, v.normal.y, v.normal.z);
                    rlColor4ub(c.r, c.g, c.b, c.a);
                    rlVertex3f(v.position.x, v.position.y, v.position.z);
                };

                emitVertex(v0);
                emitVertex(v1);
                emitVertex(v2);
            }
        }

        rlEnd();
    }

    // 2. Wireframe overlay
    if (showWireframe3D) {
        rlEnableWireMode();
        rlBegin(RL_TRIANGLES);
        Color wireCol = { 20, 24, 30, 200 };

        for (const auto& patch : currentSkin3D.patches) {
            for (size_t idx = 0; idx + 2 < patch.indices.size(); idx += 3) {
                const auto& v0 = patch.vertices[patch.indices[idx]];
                const auto& v1 = patch.vertices[patch.indices[idx + 1]];
                const auto& v2 = patch.vertices[patch.indices[idx + 2]];

                rlColor4ub(wireCol.r, wireCol.g, wireCol.b, wireCol.a);
                rlVertex3f(v0.position.x, v0.position.y, v0.position.z);
                rlVertex3f(v1.position.x, v1.position.y, v1.position.z);
                rlVertex3f(v2.position.x, v2.position.y, v2.position.z);
            }
        }
        rlEnd();
        rlDisableWireMode();
    }

    rlEnableBackfaceCulling();
}

void TubularDemo::renderScene3D() {
    if (showGrid3D) {
        DrawGrid(20, 0.5f);
    }

    if (currentAlgorithm == TubularAlgorithm::DISK_BEZIER) {
        if (currentDisk3D.spineCurve.size() >= 2) {
            for (size_t i = 0; i + 1 < currentDisk3D.spineCurve.size(); ++i) {
                DrawLine3D(currentDisk3D.spineCurve[i], currentDisk3D.spineCurve[i + 1], Color{ 241, 196, 15, 180 });
            }
        }
    } else {
        if (spheres.size() >= 2) {
            for (size_t i = 0; i + 1 < spheres.size(); ++i) {
                DrawLine3D(spheres[i].center, spheres[i + 1].center, Color{ 120, 140, 165, 140 });
            }
        }
    }

    // Draw 3D surface mesh
    draw3DSurfaceMesh();

    // Draw longitudinal ribs (meridians)
    if (showLongitudinalRibs3D) {
        draw3DLongitudinalRibs();
    }

    // Draw 3D touching circles
    if (showTouchingCircles3D) {
        draw3DTouchingCircles();
    }

    // Draw 3D spheres
    if (showSpheres3D) {
        draw3DSpheres();
    }

    drawControlPolygon3D();
}

void TubularDemo::render() {
    if (!ImGui::GetIO().WantCaptureMouse) {
        if (currentMode == ViewMode::MODE_2D) {
            handleInput2D();
        } else {
            handleInput3D();
        }
    }

    ClearBackground(Color{ 22, 26, 34, 255 });

    if (currentMode == ViewMode::MODE_2D) {
        // Direct pixel space rendering via worldToScreen2D (with +Y pointing UP)
        renderScene2D();
    } else {
        BeginMode3D(camera3D);
        renderScene3D();
        EndMode3D();
    }

    // Status overlay banner at bottom
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();
    DrawRectangle(0, screenH - 32, screenW, 32, Color{ 14, 17, 23, 215 });
    DrawText(statusMessage.c_str(), 16, screenH - 24, 15, Color{ 220, 225, 235, 255 });
}

void TubularDemo::renderUI() {
    if (ImGui::CollapsingHeader("Algorithm Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        int algoIdx = (currentAlgorithm == TubularAlgorithm::KUNKLI_HOFFMANN) ? 0 : 1;
        const char* algos[] = { "Kunkli-Hoffmann Skinning", "Disk Bézier Curve" };

        if (auto grid = ui::PropertyGrid::begin("AlgoGrid")) {
            if (grid.combo("Algorithm", &algoIdx, algos, IM_ARRAYSIZE(algos))) {
                currentAlgorithm = (algoIdx == 0) ? TubularAlgorithm::KUNKLI_HOFFMANN : TubularAlgorithm::DISK_BEZIER;
                recomputeSkin();
            }

            if (currentAlgorithm == TubularAlgorithm::DISK_BEZIER) {
                int degIdx = (bezierDegreeMode == core::BezierDegreeMode::PIECEWISE_CUBIC_SPLINE) ? 0 : 1;
                const char* degModes[] = { "Piecewise Cubic Spline (C1)", "Single Bézier (Hull)" };
                if (grid.combo("Bézier Mode", &degIdx, degModes, IM_ARRAYSIZE(degModes))) {
                    bezierDegreeMode = (degIdx == 0) ? core::BezierDegreeMode::PIECEWISE_CUBIC_SPLINE : core::BezierDegreeMode::SINGLE_BEZIER;
                    recomputeSkin();
                }

                grid.checkbox("Control Poly", &showControlPolygon);

                const auto& diag = (currentMode == ViewMode::MODE_2D) ? currentDisk2D.diagnostics : currentDisk3D.diagnostics;
                grid.text("Degree", "%d", diag.degree);
                grid.text("Margin", "%.3f", diag.minMargin);
            }
        }

        if (currentAlgorithm == TubularAlgorithm::DISK_BEZIER) {
            const auto& diag = (currentMode == ViewMode::MODE_2D) ? currentDisk2D.diagnostics : currentDisk3D.diagnostics;
            if (diag.hasSwallowing) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.65f, 0.0f, 1.0f));
                ImGui::TextWrapped("Notice: Swallowing condition detected (||c'|| < |r'|)");
                ImGui::PopStyleColor();
            }
        }
    }

    if (ImGui::CollapsingHeader("View Mode Selection", ImGuiTreeNodeFlags_DefaultOpen)) {
        int modeIdx = (currentMode == ViewMode::MODE_2D) ? 0 : 1;
        const char* modes[] = { "2D Canvas (Click & Edit Circles)", "3D Tubular Surface" };
        if (auto grid = ui::PropertyGrid::begin("ViewModeGrid")) {
            if (grid.combo("View Mode", &modeIdx, modes, IM_ARRAYSIZE(modes))) {
                currentMode = (modeIdx == 0) ? ViewMode::MODE_2D : ViewMode::MODE_3D;
                statusMessage = (currentMode == ViewMode::MODE_2D)
                    ? "Switched to 2D Canvas: Click to place circles, drag to edit."
                    : "Switched to 3D View: Left-drag to orbit, right-drag to pan, wheel to zoom.";
            }
        }

        if (currentMode == ViewMode::MODE_2D) {
            if (ImGui::Button("Switch to 3D Surface View ->", ImVec2(-1, 26))) {
                currentMode = ViewMode::MODE_3D;
                statusMessage = "Switched to 3D Surface View.";
            }
            if (ImGui::Button("Reset 2D Canvas View", ImVec2(-1, 0))) {
                resetCamera2D();
            }
        } else {
            if (ImGui::Button("<- Switch to 2D Circle Canvas", ImVec2(-1, 26))) {
                currentMode = ViewMode::MODE_2D;
                statusMessage = "Switched to 2D Circle Canvas.";
            }
            if (ImGui::Button("Reset 3D Orbit Camera", ImVec2(-1, 0))) {
                resetCamera3D();
            }
        }
    }

    if (ImGui::CollapsingHeader("Presets (Kunkli-Hoffmann Paper)", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (ImGui::Button("S-Curve Pipe", ImVec2(-1, 0))) {
            loadPreset(0);
        }
        if (ImGui::Button("Varying Radii", ImVec2(-1, 0))) {
            loadPreset(1);
        }
        if (ImGui::Button("Space Knot", ImVec2(-1, 0))) {
            loadPreset(2);
        }
        if (ImGui::Button("Seahorse", ImVec2(-1, 0))) {
            loadPreset(3);
        }
    }

    if (currentMode == ViewMode::MODE_2D) {
        if (ImGui::CollapsingHeader("2D Visualizations", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show Input Circles", &showCircles2D);
            ImGui::Checkbox("Show Labels", &showLabels2D);
            ImGui::Checkbox("Show Grid", &showGrid2D);
            ImGui::Checkbox("Hermite Skin Curves (Green: Left, Orange: Right)", &showSkinCurves2D);
            ImGui::Checkbox("Filled Tube Ribbon", &showFilledSkin2D);
            ImGui::Checkbox("Contact Points (p_i, p_bar_i)", &showContactPoints2D);
            ImGui::Checkbox("Tangent Vectors (v_i, v_bar_i)", &showTangents2D);
            ImGui::Checkbox("Radical Lines (M_i)", &showRadicalLines2D);
            ImGui::Checkbox("Radical Centers (R_i)", &showRadicalCenters2D);
        }
    } else {
        if (ImGui::CollapsingHeader("Paper Style Quick Switch", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (ImGui::Button("Paper Shaded View", ImVec2(-1, 26))) {
                applyPaperShadedStyle();
            }
            if (ImGui::Button("Paper Structural View", ImVec2(-1, 26))) {
                applyPaperStructuralStyle();
            }
        }

        if (ImGui::CollapsingHeader("3D Surface Visualizations", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Show 3D Tubular Surface", &showSurface3D);
            ImGui::Checkbox("Wireframe Overlay", &showWireframe3D);
            ImGui::Checkbox("Show Longitudinal Meridians (Blue Ribs)", &showLongitudinalRibs3D);
            ImGui::Checkbox("Show 3D Touching Circles (Green Rings)", &showTouchingCircles3D);
            ImGui::Checkbox("Show 3D Spheres", &showSpheres3D);
            ImGui::Checkbox("Show Ground Grid", &showGrid3D);
            if (auto grid = ui::PropertyGrid::begin("SurfaceVisualsGrid")) {
                grid.colorEdit3("Color", surfaceColor);
                grid.sliderFloat("Intensity", &lightIntensity, 0.2f, 2.0f);
                grid.sliderFloat("Shininess", &shininess, 8.0f, 64.0f, "%.0f");
                grid.sliderFloat("Specular", &specularStrength, 0.0f, 1.0f, "%.2f");
            }
        }
    }

    if (ImGui::CollapsingHeader("Shape & Resolution Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (auto grid = ui::PropertyGrid::begin("ShapeResolutionGrid")) {
            if (currentAlgorithm == TubularAlgorithm::KUNKLI_HOFFMANN) {
                if (grid.sliderFloat("Tangent Scale", &tangentScale, 0.5f, 4.0f, "%.2f")) {
                    recomputeSkin();
                }
            }
            if (currentMode == ViewMode::MODE_2D) {
                if (grid.sliderInt("Curve Res", &curveResolution2D, 16, 128)) recomputeSkin();
            } else {
                if (grid.sliderInt("Angular Slices", &phiResolution3D, 16, 72)) recomputeSkin();
                if (grid.sliderInt("Long. Steps", &tResolution3D, 8, 48)) recomputeSkin();
            }
        }
    }

    if (ImGui::CollapsingHeader("Spheres / Circles List", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Total items: %zu", spheres.size());

        if (ImGui::Button("+ Add New Sphere at Origin", ImVec2(-1, 0))) {
            float z = (currentMode == ViewMode::MODE_3D && !spheres.empty()) ? spheres.back().center.z + 0.5f : 0.0f;
            float x = spheres.empty() ? 0.0f : spheres.back().center.x + 0.8f;
            spheres.push_back({ { x, 0.0f, z }, 0.4f });
            selectedSphereIndex = (int)spheres.size() - 1;
            recomputeSkin();
        }
        if (ImGui::Button("Clear All Spheres/Circles", ImVec2(-1, 0))) {
            spheres.clear();
            selectedSphereIndex = -1;
            recomputeSkin();
            statusMessage = "Cleared all spheres. Add circles in 2D or spheres in 3D.";
        }

        for (size_t i = 0; i < spheres.size(); ++i) {
            std::string label = "Sphere / Circle " + std::to_string(i + 1);
            if (ui::TreeNodeScope node{label.c_str()}) {
                auto& s = spheres[i];
                float pos[3] = { s.center.x, s.center.y, s.center.z };
                if (auto grid = ui::PropertyGrid::begin("SphereProps")) {
                    if (grid.dragFloat3("Center", pos, 0.02f)) {
                        s.center = { pos[0], pos[1], pos[2] };
                        recomputeSkin();
                    }
                    if (grid.sliderFloat("Radius", &s.radius, 0.05f, 2.0f, "%.2f")) {
                        recomputeSkin();
                    }
                }
                if (ImGui::Button(("Delete##" + std::to_string(i)).c_str())) {
                    spheres.erase(spheres.begin() + i);
                    if (selectedSphereIndex == (int)i) selectedSphereIndex = -1;
                    recomputeSkin();
                    break;
                }
            }
        }
    }
}
