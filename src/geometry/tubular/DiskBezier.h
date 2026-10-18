#pragma once

#include "geometry/tubular/CircleSkinning.h"
#include <raylib.h>
#include <vector>
#include <string>

namespace core {

enum class BezierDegreeMode {
    PIECEWISE_CUBIC_SPLINE = 0,
    SINGLE_BEZIER = 1
};

enum class EndCapStyle {
    ROUND = 0,
    FLAT = 1,
    OPEN = 2
};

struct DiskBezierConfig2D {
    BezierDegreeMode mode{ BezierDegreeMode::PIECEWISE_CUBIC_SPLINE };
    int samplesPerSegment{ 48 };
    EndCapStyle endCapStyle{ EndCapStyle::ROUND };
};

struct DiskBezierConfig3D {
    BezierDegreeMode mode{ BezierDegreeMode::PIECEWISE_CUBIC_SPLINE };
    int tSamplesPerSegment{ 24 };
    int phiResolution{ 36 };
    EndCapStyle endCapStyle{ EndCapStyle::OPEN };
};

struct DiskBezierDiagnostics {
    bool hasSwallowing{ false };
    float minMargin{ 0.0f };
    int degree{ 0 };
};

struct DiskBezierResult2D {
    SkinningResult skin;
    DiskBezierDiagnostics diagnostics;
    std::vector<Vector2> controlPolygon;
    std::vector<Vector2> spineCurve;
};

struct DiskBezierResult3D {
    SphereSkinningResult3D skin;
    DiskBezierDiagnostics diagnostics;
    std::vector<Vector3> controlPolygon;
    std::vector<Vector3> spineCurve;
};

class DiskBezier {
public:
    static DiskBezierResult2D computeSkinning2D(
        const std::vector<CircleData>& circles,
        const DiskBezierConfig2D& config = {}
    );

    static DiskBezierResult3D computeSkinning3D(
        const std::vector<SphereData3D>& spheres,
        const DiskBezierConfig3D& config = {}
    );

    static void evaluateBezier2D(
        const std::vector<CircleData>& controlDisks,
        float t,
        Vector2& outC, float& outR,
        Vector2& outCDeriv, float& outRDeriv
    );

    static void evaluateBezier3D(
        const std::vector<SphereData3D>& controlBalls,
        float t,
        Vector3& outB, float& outR,
        Vector3& outBDeriv, float& outRDeriv
    );

    static bool evaluateEnvelopePoint2D(
        const Vector2& c, float r,
        const Vector2& cDeriv, float rDeriv,
        Vector2& outLeft, Vector2& outRight,
        bool& outIsSwallowed
    );

    static bool evaluateContactCircle3D(
        const Vector3& b, float r,
        const Vector3& bDeriv, float rDeriv,
        Vector3& outCenter, float& outRadius, Vector3& outNormal,
        bool& outIsSwallowed
    );
};

} // namespace core
