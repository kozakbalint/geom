#include "geometry/tubular/DiskBezier.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>
#include <limits>

namespace core {

namespace {

Vector2 NormalizeSafe(const Vector2& v, const Vector2& fallback = { 1.0f, 0.0f }) {
    float len = Vector2Length(v);
    return (len > 1e-6f) ? Vector2Scale(v, 1.0f / len) : fallback;
}

Vector3 NormalizeSafe(const Vector3& v, const Vector3& fallback = { 0.0f, 1.0f, 0.0f }) {
    float len = Vector3Length(v);
    return (len > 1e-6f) ? Vector3Scale(v, 1.0f / len) : fallback;
}

std::vector<Vector2> Build2DEndCap(
    const Vector2& center,
    float radius,
    const Vector2& pLeft,
    const Vector2& pRight,
    const Vector2& outwardDir
) {
    Vector2 vL = NormalizeSafe(Vector2Subtract(pLeft, center));
    Vector2 vR = NormalizeSafe(Vector2Subtract(pRight, center));
    float aL = atan2f(vL.y, vL.x);
    float aR = atan2f(vR.y, vR.x);

    float span = aR - aL;
    while (span < 0.0f) span += 2.0f * PI;
    while (span >= 2.0f * PI) span -= 2.0f * PI;

    // Direct the arc outward away from the curve body
    float midAngle = aL + span * 0.5f;
    Vector2 midDir = { cosf(midAngle), sinf(midAngle) };
    if (Vector2DotProduct(midDir, outwardDir) < 0.0f) {
        aL = aR;
        span = 2.0f * PI - span;
    }

    const int capRes = 24;
    std::vector<Vector2> pts(capRes + 1);
    for (int k = 0; k <= capRes; ++k) {
        float angle = aL + span * ((float)k / (float)capRes);
        pts[k] = { center.x + cosf(angle) * radius, center.y + sinf(angle) * radius };
    }
    return pts;
}

} // anonymous namespace

void DiskBezier::evaluateBezier2D(
    const std::vector<CircleData>& controlDisks,
    float t,
    Vector2& outC, float& outR,
    Vector2& outCDeriv, float& outRDeriv
) {
    size_t nPoints = controlDisks.size();
    if (nPoints == 0) {
        outC = { 0.0f, 0.0f };
        outR = 0.0f;
        outCDeriv = { 0.0f, 0.0f };
        outRDeriv = 0.0f;
        return;
    }
    if (nPoints == 1) {
        outC = controlDisks[0].center;
        outR = controlDisks[0].radius;
        outCDeriv = { 0.0f, 0.0f };
        outRDeriv = 0.0f;
        return;
    }

    // Affine representation in R^(2+1) allows evaluating center and radius in a single De Casteljau pyramid
    struct DiskPt {
        float x, y, r;
    };
    std::vector<DiskPt> pts(nPoints);
    for (size_t i = 0; i < nPoints; ++i) {
        pts[i] = { controlDisks[i].center.x, controlDisks[i].center.y, controlDisks[i].radius };
    }

    int n = static_cast<int>(nPoints) - 1;
    DiskPt p0_prev = pts[0];
    DiskPt p1_prev = pts[1];

    for (int j = 1; j <= n; ++j) {
        if (j == n) {
            p0_prev = pts[0];
            p1_prev = pts[1];
        }
        for (int i = 0; i <= n - j; ++i) {
            pts[i].x = (1.0f - t) * pts[i].x + t * pts[i + 1].x;
            pts[i].y = (1.0f - t) * pts[i].y + t * pts[i + 1].y;
            pts[i].r = (1.0f - t) * pts[i].r + t * pts[i + 1].r;
        }
    }

    outC = { pts[0].x, pts[0].y };
    outR = std::max(0.001f, pts[0].r);

    // Exact derivative via the final two intermediate points in De Casteljau: P'(t) = n * (P_1^(n-1) - P_0^(n-1))
    outCDeriv = {
        static_cast<float>(n) * (p1_prev.x - p0_prev.x),
        static_cast<float>(n) * (p1_prev.y - p0_prev.y)
    };
    outRDeriv = static_cast<float>(n) * (p1_prev.r - p0_prev.r);
}

void DiskBezier::evaluateBezier3D(
    const std::vector<SphereData3D>& controlBalls,
    float t,
    Vector3& outB, float& outR,
    Vector3& outBDeriv, float& outRDeriv
) {
    size_t nPoints = controlBalls.size();
    if (nPoints == 0) {
        outB = { 0.0f, 0.0f, 0.0f };
        outR = 0.0f;
        outBDeriv = { 0.0f, 0.0f, 0.0f };
        outRDeriv = 0.0f;
        return;
    }
    if (nPoints == 1) {
        outB = controlBalls[0].center;
        outR = controlBalls[0].radius;
        outBDeriv = { 0.0f, 0.0f, 0.0f };
        outRDeriv = 0.0f;
        return;
    }

    // Affine representation in R^(3+1)
    struct BallPt {
        float x, y, z, r;
    };
    std::vector<BallPt> pts(nPoints);
    for (size_t i = 0; i < nPoints; ++i) {
        pts[i] = { controlBalls[i].center.x, controlBalls[i].center.y, controlBalls[i].center.z, controlBalls[i].radius };
    }

    int n = static_cast<int>(nPoints) - 1;
    BallPt p0_prev = pts[0];
    BallPt p1_prev = pts[1];

    for (int j = 1; j <= n; ++j) {
        if (j == n) {
            p0_prev = pts[0];
            p1_prev = pts[1];
        }
        for (int i = 0; i <= n - j; ++i) {
            pts[i].x = (1.0f - t) * pts[i].x + t * pts[i + 1].x;
            pts[i].y = (1.0f - t) * pts[i].y + t * pts[i + 1].y;
            pts[i].z = (1.0f - t) * pts[i].z + t * pts[i + 1].z;
            pts[i].r = (1.0f - t) * pts[i].r + t * pts[i + 1].r;
        }
    }

    outB = { pts[0].x, pts[0].y, pts[0].z };
    outR = std::max(0.001f, pts[0].r);

    outBDeriv = {
        static_cast<float>(n) * (p1_prev.x - p0_prev.x),
        static_cast<float>(n) * (p1_prev.y - p0_prev.y),
        static_cast<float>(n) * (p1_prev.z - p0_prev.z)
    };
    outRDeriv = static_cast<float>(n) * (p1_prev.r - p0_prev.r);
}

bool DiskBezier::evaluateEnvelopePoint2D(
    const Vector2& c, float r,
    const Vector2& cDeriv, float rDeriv,
    Vector2& outLeft, Vector2& outRight,
    bool& outIsSwallowed
) {
    float D = cDeriv.x * cDeriv.x + cDeriv.y * cDeriv.y;
    if (D < 1e-12f) {
        outLeft = { c.x, c.y + r };
        outRight = { c.x, c.y - r };
        outIsSwallowed = (fabsf(rDeriv) > 0.0f);
        return false;
    }

    // Lin & Rokne (1998) Section 7: W = D - r'^2 < 0 denotes swallowing condition
    float W = D - rDeriv * rDeriv;
    if (W < 0.0f) {
        outIsSwallowed = true;
        W = 0.0f;
    } else {
        outIsSwallowed = false;
    }

    float sqrtW = sqrtf(W);
    float invD = 1.0f / D;

    float uLeft = (-cDeriv.x * rDeriv - cDeriv.y * sqrtW) * invD;
    float vLeft = (-cDeriv.y * rDeriv + cDeriv.x * sqrtW) * invD;

    float uRight = (-cDeriv.x * rDeriv + cDeriv.y * sqrtW) * invD;
    float vRight = (-cDeriv.y * rDeriv - cDeriv.x * sqrtW) * invD;

    outLeft = { c.x + uLeft * r, c.y + vLeft * r };
    outRight = { c.x + uRight * r, c.y + vRight * r };

    return !outIsSwallowed;
}

bool DiskBezier::evaluateContactCircle3D(
    const Vector3& b, float r,
    const Vector3& bDeriv, float rDeriv,
    Vector3& outCenter, float& outRadius, Vector3& outNormal,
    bool& outIsSwallowed
) {
    float D = bDeriv.x * bDeriv.x + bDeriv.y * bDeriv.y + bDeriv.z * bDeriv.z;
    if (D < 1e-12f) {
        outCenter = b;
        outRadius = r;
        outNormal = { 0.0f, 1.0f, 0.0f };
        outIsSwallowed = (fabsf(rDeriv) > 0.0f);
        return false;
    }

    float lenB = sqrtf(D);
    outNormal = { bDeriv.x / lenB, bDeriv.y / lenB, bDeriv.z / lenB };

    float W = D - rDeriv * rDeriv;
    if (W < 0.0f) {
        outIsSwallowed = true;
        W = 0.0f;
    } else {
        outIsSwallowed = false;
    }

    // Lin & Rokne Section 9: contact circle center along generator ball tangent
    float factor = (r * rDeriv) / D;
    outCenter = {
        b.x - bDeriv.x * factor,
        b.y - bDeriv.y * factor,
        b.z - bDeriv.z * factor
    };

    outRadius = (r / lenB) * sqrtf(W);
    return !outIsSwallowed;
}

DiskBezierResult2D DiskBezier::computeSkinning2D(
    const std::vector<CircleData>& circles,
    const DiskBezierConfig2D& config
) {
    DiskBezierResult2D result;
    size_t n = circles.size();
    if (n < 2) {
        result.skin.isValid = false;
        result.skin.errorMessage = "Add at least 2 circles to compute skinning.";
        return result;
    }

    result.diagnostics.hasSwallowing = false;
    result.diagnostics.minMargin = std::numeric_limits<float>::infinity();

    if (config.mode == BezierDegreeMode::SINGLE_BEZIER) {
        result.diagnostics.degree = static_cast<int>(n) - 1;

        result.controlPolygon.reserve(n);
        for (const auto& c : circles) {
            result.controlPolygon.push_back(c.center);
        }

        int totalSamples = std::max(16, config.samplesPerSegment * static_cast<int>(n - 1));
        result.skin.segments.resize(1);
        auto& seg = result.skin.segments[0];
        seg.leftCurvePoints.reserve(totalSamples + 1);
        seg.rightCurvePoints.reserve(totalSamples + 1);
        result.spineCurve.reserve(totalSamples + 1);

        for (int k = 0; k <= totalSamples; ++k) {
            float t = static_cast<float>(k) / static_cast<float>(totalSamples);
            Vector2 c, cDeriv;
            float r, rDeriv;
            evaluateBezier2D(circles, t, c, r, cDeriv, rDeriv);

            result.spineCurve.push_back(c);

            float speed = Vector2Length(cDeriv);
            float margin = speed - fabsf(rDeriv);
            result.diagnostics.minMargin = std::min(result.diagnostics.minMargin, margin);
            if (margin < 0.0f) {
                result.diagnostics.hasSwallowing = true;
            }

            Vector2 pLeft, pRight;
            bool swallowed = false;
            evaluateEnvelopePoint2D(c, r, cDeriv, rDeriv, pLeft, pRight, swallowed);
            seg.leftCurvePoints.push_back(pLeft);
            seg.rightCurvePoints.push_back(pRight);
        }

        result.skin.leftPoints.resize(n);
        result.skin.rightPoints.resize(n);
        for (size_t i = 0; i < n; ++i) {
            float t = static_cast<float>(i) / static_cast<float>(n - 1);
            Vector2 c, cDeriv;
            float r, rDeriv;
            evaluateBezier2D(circles, t, c, r, cDeriv, rDeriv);

            Vector2 pLeft, pRight;
            bool swallowed = false;
            evaluateEnvelopePoint2D(c, r, cDeriv, rDeriv, pLeft, pRight, swallowed);

            Vector2 tanVec = Vector2Scale(NormalizeSafe(cDeriv), 0.35f);
            result.skin.leftPoints[i] = { pLeft, tanVec };
            result.skin.rightPoints[i] = { pRight, tanVec };
        }
    } else {
        // Piecewise Cubic Spline in R^(2+1) with Catmull-Rom tangents for C1 continuity
        result.diagnostics.degree = 3;
        size_t nSegments = n - 1;

        struct Pt3D {
            float x, y, r;
        };
        std::vector<Pt3D> pts(n);
        for (size_t i = 0; i < n; ++i) {
            pts[i] = { circles[i].center.x, circles[i].center.y, circles[i].radius };
        }

        std::vector<Pt3D> v(n);
        if (n == 2) {
            Pt3D d = { pts[1].x - pts[0].x, pts[1].y - pts[0].y, pts[1].r - pts[0].r };
            v[0] = d;
            v[1] = d;
        } else {
            v[0] = { pts[1].x - pts[0].x, pts[1].y - pts[0].y, pts[1].r - pts[0].r };
            v[n - 1] = { pts[n - 1].x - pts[n - 2].x, pts[n - 1].y - pts[n - 2].y, pts[n - 1].r - pts[n - 2].r };
            for (size_t i = 1; i < n - 1; ++i) {
                v[i] = {
                    0.5f * (pts[i + 1].x - pts[i - 1].x),
                    0.5f * (pts[i + 1].y - pts[i - 1].y),
                    0.5f * (pts[i + 1].r - pts[i - 1].r)
                };
            }
        }

        result.controlPolygon.reserve(3 * nSegments + 1);
        result.skin.segments.resize(nSegments);
        result.skin.leftPoints.resize(n);
        result.skin.rightPoints.resize(n);

        int samples = std::max(4, config.samplesPerSegment);

        for (size_t i = 0; i < nSegments; ++i) {
            // Cubic control disks for segment i
            std::vector<CircleData> bDisks(4);
            bDisks[0] = { { pts[i].x, pts[i].y }, std::max(0.001f, pts[i].r) };
            bDisks[1] = {
                { pts[i].x + (1.0f / 3.0f) * v[i].x, pts[i].y + (1.0f / 3.0f) * v[i].y },
                std::max(0.001f, pts[i].r + (1.0f / 3.0f) * v[i].r)
            };
            bDisks[2] = {
                { pts[i + 1].x - (1.0f / 3.0f) * v[i + 1].x, pts[i + 1].y - (1.0f / 3.0f) * v[i + 1].y },
                std::max(0.001f, pts[i + 1].r - (1.0f / 3.0f) * v[i + 1].r)
            };
            bDisks[3] = { { pts[i + 1].x, pts[i + 1].y }, std::max(0.001f, pts[i + 1].r) };

            if (i == 0) {
                result.controlPolygon.push_back(bDisks[0].center);
            }
            result.controlPolygon.push_back(bDisks[1].center);
            result.controlPolygon.push_back(bDisks[2].center);
            result.controlPolygon.push_back(bDisks[3].center);

            auto& seg = result.skin.segments[i];
            seg.leftCurvePoints.reserve(samples + 1);
            seg.rightCurvePoints.reserve(samples + 1);

            for (int k = 0; k <= samples; ++k) {
                float u = static_cast<float>(k) / static_cast<float>(samples);
                Vector2 c, cDeriv;
                float r, rDeriv;
                evaluateBezier2D(bDisks, u, c, r, cDeriv, rDeriv);

                if (i == 0 || k > 0) {
                    result.spineCurve.push_back(c);
                }

                float speed = Vector2Length(cDeriv);
                float margin = speed - fabsf(rDeriv);
                result.diagnostics.minMargin = std::min(result.diagnostics.minMargin, margin);
                if (margin < 0.0f) {
                    result.diagnostics.hasSwallowing = true;
                }

                Vector2 pLeft, pRight;
                bool swallowed = false;
                evaluateEnvelopePoint2D(c, r, cDeriv, rDeriv, pLeft, pRight, swallowed);
                seg.leftCurvePoints.push_back(pLeft);
                seg.rightCurvePoints.push_back(pRight);

                Vector2 tanVec = Vector2Scale(NormalizeSafe(cDeriv), 0.35f);
                if (k == 0) {
                    result.skin.leftPoints[i] = { pLeft, tanVec };
                    result.skin.rightPoints[i] = { pRight, tanVec };
                }
                if (k == samples && i == nSegments - 1) {
                    result.skin.leftPoints[i + 1] = { pLeft, tanVec };
                    result.skin.rightPoints[i + 1] = { pRight, tanVec };
                }
            }
        }
    }

    if (!result.skin.segments.empty()) {
        const auto& firstSeg = result.skin.segments.front();
        const auto& lastSeg = result.skin.segments.back();

        if (config.endCapStyle == EndCapStyle::ROUND) {
            Vector2 awayStart = NormalizeSafe(Vector2Subtract(circles[0].center, circles[1].center));
            result.skin.startCapPoints = Build2DEndCap(
                circles[0].center, circles[0].radius,
                firstSeg.leftCurvePoints.front(), firstSeg.rightCurvePoints.front(),
                awayStart
            );

            Vector2 awayEnd = NormalizeSafe(Vector2Subtract(circles[n - 1].center, circles[n - 2].center));
            result.skin.endCapPoints = Build2DEndCap(
                circles[n - 1].center, circles[n - 1].radius,
                lastSeg.leftCurvePoints.back(), lastSeg.rightCurvePoints.back(),
                awayEnd
            );
        } else if (config.endCapStyle == EndCapStyle::FLAT) {
            result.skin.startCapPoints = { firstSeg.leftCurvePoints.front(), firstSeg.rightCurvePoints.front() };
            result.skin.endCapPoints = { lastSeg.leftCurvePoints.back(), lastSeg.rightCurvePoints.back() };
        }
    }

    result.skin.isValid = true;
    return result;
}

DiskBezierResult3D DiskBezier::computeSkinning3D(
    const std::vector<SphereData3D>& spheres,
    const DiskBezierConfig3D& config
) {
    DiskBezierResult3D result;
    size_t n = spheres.size();
    if (n < 2) {
        result.skin.isValid = false;
        result.skin.errorMessage = "Add at least 2 spheres to compute skinning.";
        return result;
    }

    result.diagnostics.hasSwallowing = false;
    result.diagnostics.minMargin = std::numeric_limits<float>::infinity();

    size_t nSegments = n - 1;
    int samplesPerSeg = std::max(4, config.tSamplesPerSegment);
    int M = std::max(8, config.phiResolution);

    struct BallSample {
        Vector3 b;
        float r;
        Vector3 cCirc;
        float rCirc;
        Vector3 normal;
    };

    std::vector<BallSample> samples;
    std::vector<std::vector<SphereData3D>> segmentBeziers;

    if (config.mode == BezierDegreeMode::SINGLE_BEZIER) {
        result.diagnostics.degree = static_cast<int>(n) - 1;
        result.controlPolygon.reserve(n);
        for (const auto& s : spheres) {
            result.controlPolygon.push_back(s.center);
        }

        int totalTSteps = samplesPerSeg * static_cast<int>(nSegments);
        samples.reserve(totalTSteps + 1);

        for (int k = 0; k <= totalTSteps; ++k) {
            float t = static_cast<float>(k) / static_cast<float>(totalTSteps);
            Vector3 b, bDeriv;
            float r, rDeriv;
            evaluateBezier3D(spheres, t, b, r, bDeriv, rDeriv);

            result.spineCurve.push_back(b);

            float speed = Vector3Length(bDeriv);
            float margin = speed - fabsf(rDeriv);
            result.diagnostics.minMargin = std::min(result.diagnostics.minMargin, margin);
            if (margin < 0.0f) {
                result.diagnostics.hasSwallowing = true;
            }

            Vector3 cCirc, normal;
            float rCirc;
            bool swallowed = false;
            evaluateContactCircle3D(b, r, bDeriv, rDeriv, cCirc, rCirc, normal, swallowed);
            samples.push_back({ b, r, cCirc, rCirc, normal });
        }
    } else {
        result.diagnostics.degree = 3;

        struct Pt4D {
            float x, y, z, r;
        };
        std::vector<Pt4D> pts(n);
        for (size_t i = 0; i < n; ++i) {
            pts[i] = { spheres[i].center.x, spheres[i].center.y, spheres[i].center.z, spheres[i].radius };
        }

        std::vector<Pt4D> v(n);
        if (n == 2) {
            Pt4D d = { pts[1].x - pts[0].x, pts[1].y - pts[0].y, pts[1].z - pts[0].z, pts[1].r - pts[0].r };
            v[0] = d;
            v[1] = d;
        } else {
            v[0] = { pts[1].x - pts[0].x, pts[1].y - pts[0].y, pts[1].z - pts[0].z, pts[1].r - pts[0].r };
            v[n - 1] = { pts[n - 1].x - pts[n - 2].x, pts[n - 1].y - pts[n - 2].y, pts[n - 1].z - pts[n - 2].z, pts[n - 1].r - pts[n - 2].r };
            for (size_t i = 1; i < n - 1; ++i) {
                v[i] = {
                    0.5f * (pts[i + 1].x - pts[i - 1].x),
                    0.5f * (pts[i + 1].y - pts[i - 1].y),
                    0.5f * (pts[i + 1].z - pts[i - 1].z),
                    0.5f * (pts[i + 1].r - pts[i - 1].r)
                };
            }
        }

        result.controlPolygon.reserve(3 * nSegments + 1);
        segmentBeziers.resize(nSegments);

        for (size_t i = 0; i < nSegments; ++i) {
            auto& bBalls = segmentBeziers[i];
            bBalls.resize(4);
            bBalls[0] = { { pts[i].x, pts[i].y, pts[i].z }, std::max(0.001f, pts[i].r) };
            bBalls[1] = {
                { pts[i].x + (1.0f / 3.0f) * v[i].x, pts[i].y + (1.0f / 3.0f) * v[i].y, pts[i].z + (1.0f / 3.0f) * v[i].z },
                std::max(0.001f, pts[i].r + (1.0f / 3.0f) * v[i].r)
            };
            bBalls[2] = {
                { pts[i + 1].x - (1.0f / 3.0f) * v[i + 1].x, pts[i + 1].y - (1.0f / 3.0f) * v[i + 1].y, pts[i + 1].z - (1.0f / 3.0f) * v[i + 1].z },
                std::max(0.001f, pts[i + 1].r - (1.0f / 3.0f) * v[i + 1].r)
            };
            bBalls[3] = { { pts[i + 1].x, pts[i + 1].y, pts[i + 1].z }, std::max(0.001f, pts[i + 1].r) };

            if (i == 0) {
                result.controlPolygon.push_back(bBalls[0].center);
            }
            result.controlPolygon.push_back(bBalls[1].center);
            result.controlPolygon.push_back(bBalls[2].center);
            result.controlPolygon.push_back(bBalls[3].center);

            for (int k = 0; k <= samplesPerSeg; ++k) {
                if (i > 0 && k == 0) continue;

                float u = static_cast<float>(k) / static_cast<float>(samplesPerSeg);
                Vector3 b, bDeriv;
                float r, rDeriv;
                evaluateBezier3D(bBalls, u, b, r, bDeriv, rDeriv);

                result.spineCurve.push_back(b);

                float speed = Vector3Length(bDeriv);
                float margin = speed - fabsf(rDeriv);
                result.diagnostics.minMargin = std::min(result.diagnostics.minMargin, margin);
                if (margin < 0.0f) {
                    result.diagnostics.hasSwallowing = true;
                }

                Vector3 cCirc, normal;
                float rCirc;
                bool swallowed = false;
                evaluateContactCircle3D(b, r, bDeriv, rDeriv, cCirc, rCirc, normal, swallowed);
                samples.push_back({ b, r, cCirc, rCirc, normal });
            }
        }
    }

    size_t totalSamples = samples.size();
    if (totalSamples < 2) {
        result.skin.isValid = false;
        result.skin.errorMessage = "Failed to sample sufficient spine points.";
        return result;
    }

    // Rotation Minimizing Frame (Double Reflection Method, Wang et al. 2008) provides twist-free transport
    struct RMFFrame {
        Vector3 r;
        Vector3 s;
    };
    std::vector<RMFFrame> frames(totalSamples);

    Vector3 t0 = samples[0].normal;
    Vector3 ref = (fabsf(t0.y) < 0.9f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
    Vector3 r0 = NormalizeSafe(Vector3CrossProduct(t0, ref));
    Vector3 s0 = NormalizeSafe(Vector3CrossProduct(t0, r0));
    frames[0] = { r0, s0 };

    for (size_t j = 1; j < totalSamples; ++j) {
        Vector3 t_prev = samples[j - 1].normal;
        Vector3 t_curr = samples[j].normal;
        Vector3 r_prev = frames[j - 1].r;

        Vector3 v1 = Vector3Subtract(samples[j].cCirc, samples[j - 1].cCirc);
        float c1 = Vector3DotProduct(v1, v1);
        Vector3 r_curr;

        if (c1 > 1e-10f) {
            Vector3 r_prev_L = Vector3Subtract(r_prev, Vector3Scale(v1, 2.0f * Vector3DotProduct(v1, r_prev) / c1));
            Vector3 t_prev_L = Vector3Subtract(t_prev, Vector3Scale(v1, 2.0f * Vector3DotProduct(v1, t_prev) / c1));
            Vector3 v2 = Vector3Subtract(t_curr, t_prev_L);
            float c2 = Vector3DotProduct(v2, v2);
            if (c2 > 1e-10f) {
                r_curr = Vector3Subtract(r_prev_L, Vector3Scale(v2, 2.0f * Vector3DotProduct(v2, r_prev_L) / c2));
            } else {
                r_curr = r_prev_L;
            }
        } else {
            Vector3 v2 = Vector3Subtract(t_curr, t_prev);
            float c2 = Vector3DotProduct(v2, v2);
            if (c2 > 1e-10f) {
                r_curr = Vector3Subtract(r_prev, Vector3Scale(v2, 2.0f * Vector3DotProduct(v2, r_prev) / c2));
            } else {
                r_curr = r_prev;
            }
        }

        r_curr = NormalizeSafe(r_curr);
        Vector3 s_curr = NormalizeSafe(Vector3CrossProduct(t_curr, r_curr));
        frames[j] = { r_curr, s_curr };
    }

    result.skin.touchingCircles.resize(n);
    for (size_t i = 0; i < n; ++i) {
        size_t sampleIdx = std::min(i * samplesPerSeg, totalSamples - 1);
        TouchingCircle3D tc;
        tc.center = samples[sampleIdx].cCirc;
        tc.radius = samples[sampleIdx].rCirc;
        tc.normal = samples[sampleIdx].normal;
        tc.u = frames[sampleIdx].r;
        tc.v = frames[sampleIdx].s;
        result.skin.touchingCircles[i] = tc;
    }

    // Build quadrilateral surface patches per segment
    result.skin.patches.resize(nSegments);
    for (size_t segIdx = 0; segIdx < nSegments; ++segIdx) {
        auto& patch = result.skin.patches[segIdx];
        patch.phiSteps = M;
        patch.tSteps = samplesPerSeg;
        patch.vertices.resize((samplesPerSeg + 1) * (M + 1));

        for (int stepT = 0; stepT <= samplesPerSeg; ++stepT) {
            size_t globalIdx = segIdx * samplesPerSeg + stepT;
            const auto& smp = samples[globalIdx];
            const auto& frm = frames[globalIdx];

            for (int stepPhi = 0; stepPhi <= M; ++stepPhi) {
                float phi = static_cast<float>(stepPhi) * 2.0f * PI / static_cast<float>(M);
                float cosP = cosf(phi);
                float sinP = sinf(phi);

                Vector3 circDir = Vector3Add(Vector3Scale(frm.r, cosP), Vector3Scale(frm.s, sinP));
                Vector3 pos = Vector3Add(smp.cCirc, Vector3Scale(circDir, smp.rCirc));

                // Exact analytical outward normal of sphere envelope at contact point: n = (P - b) / r
                Vector3 outward = Vector3Subtract(pos, smp.b);
                Vector3 normal = NormalizeSafe(outward, circDir);

                int vIdx = stepT * (M + 1) + stepPhi;
                patch.vertices[vIdx].position = pos;
                patch.vertices[vIdx].normal = normal;
            }
        }

        for (int stepT = 0; stepT < samplesPerSeg; ++stepT) {
            for (int stepPhi = 0; stepPhi < M; ++stepPhi) {
                int i0 = stepT * (M + 1) + stepPhi;
                int i1 = stepT * (M + 1) + (stepPhi + 1);
                int i2 = (stepT + 1) * (M + 1) + (stepPhi + 1);
                int i3 = (stepT + 1) * (M + 1) + stepPhi;

                patch.indices.push_back(i0);
                patch.indices.push_back(i1);
                patch.indices.push_back(i2);

                patch.indices.push_back(i0);
                patch.indices.push_back(i2);
                patch.indices.push_back(i3);
            }
        }
    }

    result.skin.isValid = true;
    return result;
}

} // namespace core
