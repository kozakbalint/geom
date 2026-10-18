#pragma once

#include <raylib.h>
#include <vector>
#include <string>

namespace core {

struct CircleData {
    Vector2 center{ 0.0f, 0.0f };
    float radius{ 10.0f };
};

struct SkinContactPoint {
    Vector2 point{ 0.0f, 0.0f };
    Vector2 tangent{ 0.0f, 0.0f }; // Hermite tangent vector (length = lambda * dist(p, radical line))
};

struct RadicalLineData {
    Vector2 pointOnLine{ 0.0f, 0.0f };
    Vector2 normal{ 0.0f, 0.0f };      // Normalized orthogonal to center line
    Vector2 dir{ 0.0f, 0.0f };         // Direction along radical line
};

struct SkinSegment {
    std::vector<Vector2> leftCurvePoints;
    std::vector<Vector2> rightCurvePoints;
};

struct SkinningResult {
    bool isValid{ false };
    std::string errorMessage;

    // Contact points per circle (index matches input circles)
    std::vector<SkinContactPoint> leftPoints;
    std::vector<SkinContactPoint> rightPoints;

    // Geometric auxiliaries for inspection/visualization
    std::vector<Vector2> radicalCenters;          // For triplets (i = 1 .. n-2)
    std::vector<RadicalLineData> radicalLines;    // For consecutive pairs (i = 0 .. n-2)

    // Hermite segments between consecutive circles (n - 1 segments)
    std::vector<SkinSegment> segments;

    // End cap points (for closing the tubular shape)
    std::vector<Vector2> startCapPoints;
    std::vector<Vector2> endCapPoints;
};

struct SphereData3D {
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float radius{ 1.0f };
};

struct TouchingCircle3D {
    Vector3 center{ 0.0f, 0.0f, 0.0f };
    float radius{ 1.0f };
    Vector3 normal{ 0.0f, 1.0f, 0.0f };
    Vector3 u{ 1.0f, 0.0f, 0.0f };
    Vector3 v{ 0.0f, 0.0f, 1.0f };
};

struct SurfaceVertex3D {
    Vector3 position{ 0.0f, 0.0f, 0.0f };
    Vector3 normal{ 0.0f, 1.0f, 0.0f };
};

struct SurfacePatch3D {
    int phiSteps{ 36 };
    int tSteps{ 24 };
    std::vector<SurfaceVertex3D> vertices;
    std::vector<int> indices;
};

struct SphereSkinningResult3D {
    bool isValid{ false };
    std::string errorMessage;

    // Touching circles for each sphere (n circles)
    std::vector<TouchingCircle3D> touchingCircles;

    // 3D skinning surface patches (n - 1 patches)
    std::vector<SurfacePatch3D> patches;
};

class CircleSkinning {
public:
    // Computes the Kunkli-Hoffmann skinning for a sequence of 2D circles
    static SkinningResult computeSkinning(
        const std::vector<CircleData>& circles,
        float tangentScale = 2.0f,
        int curveResolution = 48
    );

    // Computes the Kunkli-Hoffmann 3D skinning surface for a sequence of 3D spheres (Section 4)
    static SphereSkinningResult3D computeSkinning3D(
        const std::vector<SphereData3D>& spheres,
        float tangentScale = 2.0f,
        int phiResolution = 36,
        int tResolution = 24
    );

    // Evaluates cubic Hermite curve in 2D at parameter t in [0, 1]
    static Vector2 evaluateHermite(
        const Vector2& p0,
        const Vector2& p1,
        const Vector2& v0,
        const Vector2& v1,
        float t
    );

    // Evaluates cubic Hermite curve in 3D at parameter t in [0, 1]
    static Vector3 evaluateHermite3D(
        const Vector3& p0,
        const Vector3& p1,
        const Vector3& v0,
        const Vector3& v1,
        float t
    );

    // Calculates the radical line between two circles
    static bool computeRadicalLine(
        const CircleData& c1,
        const CircleData& c2,
        RadicalLineData& outLine
    );

    // Calculates distance from a point to a radical line
    static float distanceToRadicalLine(
        const Vector2& point,
        const RadicalLineData& line
    );

    // Calculates radical center of 3 circles
    static bool computeRadicalCenter(
        const CircleData& c1,
        const CircleData& c2,
        const CircleData& c3,
        Vector2& outCenter
    );
};

} // namespace core
