#include "geometry/tubular/CircleSkinning.h"
#include <raymath.h>
#include <cmath>
#include <algorithm>

namespace core {

namespace {

float CrossProduct2D(const Vector2& a, const Vector2& b) {
    return a.x * b.y - a.y * b.x;
}

Vector2 NormalizeSafe(const Vector2& v, const Vector2& fallback = { 1.0f, 0.0f }) {
    float len = Vector2Length(v);
    if (len > 1e-6f) {
        return Vector2Scale(v, 1.0f / len);
    }
    return fallback;
}

// Compute external bitangent contact points on c1 for pair (c1, c2)
void ComputeExternalBitangentContacts(
    const CircleData& c1,
    const CircleData& c2,
    Vector2& outLeft,
    Vector2& outRight
) {
    Vector2 dVec = Vector2Subtract(c2.center, c1.center);
    float dist = Vector2Length(dVec);

    if (dist < 1e-5f) {
        // Degenerate concentric circles
        outLeft = { c1.center.x, c1.center.y - c1.radius };
        outRight = { c1.center.x, c1.center.y + c1.radius };
        return;
    }

    float diffR = c1.radius - c2.radius;
    float ratio = diffR / dist;
    ratio = std::clamp(ratio, -0.9999f, 0.9999f);

    float alpha = acosf(ratio);
    float baseAngle = atan2f(dVec.y, dVec.x);

    // Left skin is positive offset (+alpha), right is negative (-alpha)
    float angleLeft = baseAngle + alpha;
    float angleRight = baseAngle - alpha;

    outLeft = { c1.center.x + cosf(angleLeft) * c1.radius, c1.center.y + sinf(angleLeft) * c1.radius };
    outRight = { c1.center.x + cosf(angleRight) * c1.radius, c1.center.y + sinf(angleRight) * c1.radius };
}

} // anonymous namespace

Vector2 CircleSkinning::evaluateHermite(
    const Vector2& p0,
    const Vector2& p1,
    const Vector2& v0,
    const Vector2& v1,
    float t
) {
    float t2 = t * t;
    float t3 = t2 * t;

    // Cubic Hermite basis functions
    float h0 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h1 = -2.0f * t3 + 3.0f * t2;
    float h2 = t3 - 2.0f * t2 + t;
    float h3 = t3 - t2;

    return {
        h0 * p0.x + h1 * p1.x + h2 * v0.x + h3 * v1.x,
        h0 * p0.y + h1 * p1.y + h2 * v0.y + h3 * v1.y
    };
}

bool CircleSkinning::computeRadicalLine(
    const CircleData& c1,
    const CircleData& c2,
    RadicalLineData& outLine
) {
    Vector2 dVec = Vector2Subtract(c2.center, c1.center);
    float d = Vector2Length(dVec);
    if (d < 1e-5f) {
        return false;
    }

    // Distance from c1 center to radical axis along the center line
    float dist1 = 0.5f * (d + (c1.radius * c1.radius - c2.radius * c2.radius) / d);
    Vector2 u = Vector2Scale(dVec, 1.0f / d);

    outLine.pointOnLine = Vector2Add(c1.center, Vector2Scale(u, dist1));
    outLine.normal = u;
    outLine.dir = { -u.y, u.x };
    return true;
}

float CircleSkinning::distanceToRadicalLine(
    const Vector2& point,
    const RadicalLineData& line
) {
    // Perpendicular distance to the radical line
    Vector2 diff = Vector2Subtract(point, line.pointOnLine);
    return fabsf(Vector2DotProduct(diff, line.normal));
}

bool CircleSkinning::computeRadicalCenter(
    const CircleData& c1,
    const CircleData& c2,
    const CircleData& c3,
    Vector2& outCenter
) {
    // System of 2 radical axes:
    // 2*(o2 - o1) . R = ||o2||^2 - ||o1||^2 - (r2^2 - r1^2)
    // 2*(o3 - o2) . R = ||o3||^2 - ||o2||^2 - (r3^2 - r2^2)
    Vector2 d12 = Vector2Subtract(c2.center, c1.center);
    Vector2 d23 = Vector2Subtract(c3.center, c2.center);

    float a11 = 2.0f * d12.x;
    float a12 = 2.0f * d12.y;
    float b1 = (c2.center.x * c2.center.x + c2.center.y * c2.center.y) -
               (c1.center.x * c1.center.x + c1.center.y * c1.center.y) -
               (c2.radius * c2.radius - c1.radius * c1.radius);

    float a21 = 2.0f * d23.x;
    float a22 = 2.0f * d23.y;
    float b2 = (c3.center.x * c3.center.x + c3.center.y * c3.center.y) -
               (c2.center.x * c2.center.x + c2.center.y * c2.center.y) -
               (c3.radius * c3.radius - c2.radius * c2.radius);

    float det = a11 * a22 - a12 * a21;
    if (fabsf(det) < 1e-5f) {
        // Collinear centers
        return false;
    }

    outCenter.x = (b1 * a22 - b2 * a12) / det;
    outCenter.y = (a11 * b2 - a21 * b1) / det;
    return true;
}

SkinningResult CircleSkinning::computeSkinning(
    const std::vector<CircleData>& circles,
    float tangentScale,
    int curveResolution
) {
    SkinningResult result;
    size_t n = circles.size();

    if (n < 2) {
        result.isValid = false;
        result.errorMessage = "At least 2 circles are required for tubular skinning.";
        return result;
    }

    // 1. Compute radical lines for all consecutive pairs (n - 1 pairs)
    result.radicalLines.resize(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        if (!computeRadicalLine(circles[i], circles[i + 1], result.radicalLines[i])) {
            result.isValid = false;
            result.errorMessage = "Concentric circles detected at index " + std::to_string(i);
            return result;
        }
    }

    // 2. Compute radical centers for all triplets (n - 2 triplets)
    if (n >= 3) {
        result.radicalCenters.resize(n - 2);
        for (size_t i = 1; i < n - 1; ++i) {
            Vector2 rc;
            if (computeRadicalCenter(circles[i - 1], circles[i], circles[i + 1], rc)) {
                result.radicalCenters[i - 1] = rc;
            } else {
                // If collinear, position a virtual center far along the perpendicular bisector
                Vector2 dir = NormalizeSafe(Vector2Subtract(circles[i + 1].center, circles[i - 1].center));
                Vector2 perp = { -dir.y, dir.x };
                result.radicalCenters[i - 1] = Vector2Add(circles[i].center, Vector2Scale(perp, 10000.0f));
            }
        }
    }

    result.leftPoints.resize(n);
    result.rightPoints.resize(n);

    // 3. Contact points for the first circle (c0) with c1
    Vector2 p0_left, p0_right;
    ComputeExternalBitangentContacts(circles[0], circles[1], p0_left, p0_right);
    result.leftPoints[0].point = p0_left;
    result.rightPoints[0].point = p0_right;

    // 4. Contact points for the last circle (c_n-1) with c_n-2
    Vector2 pn_left, pn_right;
    ComputeExternalBitangentContacts(circles[n - 1], circles[n - 2], pn_left, pn_right);
    // Note: bitangent from c_n-1 to c_n-2 has reversed orientation, so left/right are flipped
    result.leftPoints[n - 1].point = pn_right;
    result.rightPoints[n - 1].point = pn_left;

    // 5. Contact points for inner circles (i = 1 .. n-2) using Apollonius circles / radical center
    for (size_t i = 1; i < n - 1; ++i) {
        const auto& cA = circles[i - 1];
        const auto& cB = circles[i];
        const auto& cC = circles[i + 1];
        Vector2 R = result.radicalCenters[i - 1];

        // Homothetic centers
        float diffAB = cA.radius - cB.radius;
        float diffBC = cB.radius - cC.radius;
        float diffAC = cA.radius - cC.radius;

        bool hasHAB = fabsf(diffAB) > 1e-4f;
        bool hasHBC = fabsf(diffBC) > 1e-4f;
        bool hasHAC = fabsf(diffAC) > 1e-4f;

        Vector2 HAB = hasHAB ? Vector2Scale(Vector2Subtract(Vector2Scale(cB.center, cA.radius), Vector2Scale(cA.center, cB.radius)), 1.0f / diffAB) : Vector2{ 0, 0 };
        Vector2 HBC = hasHBC ? Vector2Scale(Vector2Subtract(Vector2Scale(cC.center, cB.radius), Vector2Scale(cB.center, cC.radius)), 1.0f / diffBC) : Vector2{ 0, 0 };
        Vector2 HAC = hasHAC ? Vector2Scale(Vector2Subtract(Vector2Scale(cC.center, cA.radius), Vector2Scale(cA.center, cC.radius)), 1.0f / diffAC) : Vector2{ 0, 0 };

        // Axis of similitude line: A*x + B*y + C = 0
        Vector2 pole = cB.center;
        bool foundAxis = false;
        Vector2 axisPt1, axisPt2;

        if (hasHAB && hasHBC && Vector2Distance(HAB, HBC) > 1.0f) {
            axisPt1 = HAB;
            axisPt2 = HBC;
            foundAxis = true;
        } else if (hasHAB && hasHAC && Vector2Distance(HAB, HAC) > 1.0f) {
            axisPt1 = HAB;
            axisPt2 = HAC;
            foundAxis = true;
        } else if (hasHBC && hasHAC && Vector2Distance(HBC, HAC) > 1.0f) {
            axisPt1 = HBC;
            axisPt2 = HAC;
            foundAxis = true;
        } else if (hasHBC) {
            axisPt1 = HBC;
            Vector2 dAB = NormalizeSafe(Vector2Subtract(cB.center, cA.center));
            axisPt2 = Vector2Add(HBC, dAB);
            foundAxis = true;
        } else if (hasHAB) {
            axisPt1 = HAB;
            Vector2 dBC = NormalizeSafe(Vector2Subtract(cC.center, cB.center));
            axisPt2 = Vector2Add(HAB, dBC);
            foundAxis = true;
        }

        if (foundAxis) {
            Vector2 axisDir = NormalizeSafe(Vector2Subtract(axisPt2, axisPt1));
            Vector2 axisNormal = { -axisDir.y, axisDir.x };
            float dL = Vector2DotProduct(Vector2Subtract(axisPt1, cB.center), axisNormal);

            if (fabsf(dL) > 1e-3f) {
                // Pole of axis of similitude with respect to circle cB
                pole = Vector2Add(cB.center, Vector2Scale(axisNormal, (cB.radius * cB.radius) / dL));
            }
        }

        // The line through radical center R and pole P intersects circle cB at the two Apollonius contact points
        Vector2 lineDir = NormalizeSafe(Vector2Subtract(pole, R));
        if (Vector2Length(Vector2Subtract(pole, R)) < 1e-3f || fabsf(lineDir.x) < 1e-6f && fabsf(lineDir.y) < 1e-6f) {
            lineDir = NormalizeSafe(Vector2Subtract(cB.center, R));
        }

        Vector2 w = Vector2Subtract(R, cB.center);
        float B_coeff = Vector2DotProduct(lineDir, w);
        float C_coeff = Vector2DotProduct(w, w) - cB.radius * cB.radius;
        float disc = B_coeff * B_coeff - C_coeff;

        Vector2 cand1, cand2;
        if (disc >= 0.0f) {
            float sqrtDisc = sqrtf(disc);
            float s1 = -B_coeff - sqrtDisc;
            float s2 = -B_coeff + sqrtDisc;
            cand1 = Vector2Add(R, Vector2Scale(lineDir, s1));
            cand2 = Vector2Add(R, Vector2Scale(lineDir, s2));
        } else {
            // Fallback: project onto circle
            Vector2 perp = { -lineDir.y, lineDir.x };
            cand1 = Vector2Add(cB.center, Vector2Scale(perp, cB.radius));
            cand2 = Vector2Add(cB.center, Vector2Scale(perp, -cB.radius));
        }

        // Ensure points lie strictly on circle perimeter
        cand1 = Vector2Add(cB.center, Vector2Scale(NormalizeSafe(Vector2Subtract(cand1, cB.center)), cB.radius));
        cand2 = Vector2Add(cB.center, Vector2Scale(NormalizeSafe(Vector2Subtract(cand2, cB.center)), cB.radius));

        // Group into left (pi) and right (p_bar_i) based on turn orientation (Section 3.2, Fig. 7)
        Vector2 vIn = Vector2Subtract(cB.center, cA.center);
        Vector2 vOut = Vector2Subtract(cC.center, cA.center);
        float cross = CrossProduct2D(vIn, vOut);

        float dist1 = Vector2Distance(cand1, R);
        float dist2 = Vector2Distance(cand2, R);

        Vector2 p_left, p_right;
        if (cross > 1e-4f) {
            // CCW turn: point closer to radical center is in left group
            if (dist1 <= dist2) {
                p_left = cand1;
                p_right = cand2;
            } else {
                p_left = cand2;
                p_right = cand1;
            }
        } else if (cross < -1e-4f) {
            // CW turn: point closer to radical center is in right group
            if (dist1 <= dist2) {
                p_right = cand1;
                p_left = cand2;
            } else {
                p_right = cand2;
                p_left = cand1;
            }
        } else {
            // Collinear centers: test side of center vector
            Vector2 centerDir = NormalizeSafe(Vector2Subtract(cC.center, cA.center));
            Vector2 leftPerp = { -centerDir.y, centerDir.x };
            if (Vector2DotProduct(Vector2Subtract(cand1, cB.center), leftPerp) > 0.0f) {
                p_left = cand1;
                p_right = cand2;
            } else {
                p_left = cand2;
                p_right = cand1;
            }
        }

        result.leftPoints[i].point = p_left;
        result.rightPoints[i].point = p_right;
    }

    // 6. Tangent Vectors and Hermite Segments for consecutive pairs (n - 1 segments)
    result.segments.resize(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        const auto& cA = circles[i];
        const auto& cB = circles[i + 1];
        const auto& radLine = result.radicalLines[i];

        // Left side points
        Vector2 pA_left = result.leftPoints[i].point;
        Vector2 pB_left = result.leftPoints[i + 1].point;

        // Right side points
        Vector2 pA_right = result.rightPoints[i].point;
        Vector2 pB_right = result.rightPoints[i + 1].point;

        // Tangent lengths based on distance to radical line Mi (Section 3.3)
        float distA_left = distanceToRadicalLine(pA_left, radLine);
        float distB_left = distanceToRadicalLine(pB_left, radLine);
        float distA_right = distanceToRadicalLine(pA_right, radLine);
        float distB_right = distanceToRadicalLine(pB_right, radLine);

        float lenA_left = tangentScale * distA_left;
        float lenB_left = tangentScale * distB_left;
        float lenA_right = tangentScale * distA_right;
        float lenB_right = tangentScale * distB_right;

        // Tangent unit directions (tangent to circles, pointing forward towards cB)
        Vector2 fwd = Vector2Subtract(cB.center, cA.center);
        auto getTangent = [&](const Vector2& pt, const Vector2& center) {
            Vector2 rad = NormalizeSafe(Vector2Subtract(pt, center));
            Vector2 tan = { -rad.y, rad.x };
            return (Vector2DotProduct(tan, fwd) >= 0.0f) ? tan : Vector2Scale(tan, -1.0f);
        };

        Vector2 tanA_left  = getTangent(pA_left, cA.center);
        Vector2 tanB_left  = getTangent(pB_left, cB.center);
        Vector2 tanA_right = getTangent(pA_right, cA.center);
        Vector2 tanB_right = getTangent(pB_right, cB.center);

        Vector2 vA_left = Vector2Scale(tanA_left, lenA_left);
        Vector2 vB_left = Vector2Scale(tanB_left, lenB_left);
        Vector2 vA_right = Vector2Scale(tanA_right, lenA_right);
        Vector2 vB_right = Vector2Scale(tanB_right, lenB_right);

        // Store tangents on the contact points
        result.leftPoints[i].tangent = vA_left;
        result.rightPoints[i].tangent = vA_right;
        if (i == n - 2) {
            result.leftPoints[i + 1].tangent = vB_left;
            result.rightPoints[i + 1].tangent = vB_right;
        }

        // Hermite curve points
        auto& seg = result.segments[i];
        int res = std::max(8, curveResolution);
        seg.leftCurvePoints.resize(res + 1);
        seg.rightCurvePoints.resize(res + 1);

        for (int step = 0; step <= res; ++step) {
            float t = (float)step / (float)res;
            seg.leftCurvePoints[step] = evaluateHermite(pA_left, pB_left, vA_left, vB_left, t);
            seg.rightCurvePoints[step] = evaluateHermite(pA_right, pB_right, vA_right, vB_right, t);
        }
    }

    // 7. End caps along outer perimeters
    auto makeEndCap = [](const Vector2& center, float radius, const Vector2& pLeft, const Vector2& pRight, const Vector2& towardOutside) {
        Vector2 vL = NormalizeSafe(Vector2Subtract(pLeft, center));
        Vector2 vR = NormalizeSafe(Vector2Subtract(pRight, center));
        float aL = atan2f(vL.y, vL.x);
        float aR = atan2f(vR.y, vR.x);

        float span = aR - aL;
        while (span < 0.0f) span += 2.0f * PI;
        while (span >= 2.0f * PI) span -= 2.0f * PI;

        float midAngle = aL + span * 0.5f;
        Vector2 midDir = { cosf(midAngle), sinf(midAngle) };
        if (Vector2DotProduct(midDir, towardOutside) < 0.0f) {
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
    };

    Vector2 away0 = NormalizeSafe(Vector2Subtract(circles[0].center, circles[1].center));
    result.startCapPoints = makeEndCap(circles[0].center, circles[0].radius, result.leftPoints[0].point, result.rightPoints[0].point, away0);

    Vector2 awayN = NormalizeSafe(Vector2Subtract(circles[n - 1].center, circles[n - 2].center));
    result.endCapPoints = makeEndCap(circles[n - 1].center, circles[n - 1].radius, result.leftPoints[n - 1].point, result.rightPoints[n - 1].point, awayN);

    result.isValid = true;
    return result;
}

Vector3 CircleSkinning::evaluateHermite3D(
    const Vector3& p0,
    const Vector3& p1,
    const Vector3& v0,
    const Vector3& v1,
    float t
) {
    float t2 = t * t;
    float t3 = t2 * t;

    float h0 = 2.0f * t3 - 3.0f * t2 + 1.0f;
    float h1 = -2.0f * t3 + 3.0f * t2;
    float h2 = t3 - 2.0f * t2 + t;
    float h3 = t3 - t2;

    return {
        h0 * p0.x + h1 * p1.x + h2 * v0.x + h3 * v1.x,
        h0 * p0.y + h1 * p1.y + h2 * v0.y + h3 * v1.y,
        h0 * p0.z + h1 * p1.z + h2 * v0.z + h3 * v1.z
    };
}

SphereSkinningResult3D CircleSkinning::computeSkinning3D(
    const std::vector<SphereData3D>& spheres,
    float tangentScale,
    int phiResolution,
    int tResolution
) {
    SphereSkinningResult3D result;
    size_t n = spheres.size();

    if (n < 2) {
        result.isValid = false;
        result.errorMessage = "At least 2 spheres are required for 3D tubular skinning.";
        return result;
    }

    int M = std::clamp(phiResolution, 12, 96);
    int K = std::clamp(tResolution, 8, 64);

    result.touchingCircles.resize(n);

    // 1. First sphere (s0) touching circle via tangent cone with s1 (Section 4, Fig. 12)
    {
        Vector3 o0 = spheres[0].center;
        Vector3 o1 = spheres[1].center;
        float r0 = spheres[0].radius;
        float r1 = spheres[1].radius;
        Vector3 dVec = Vector3Subtract(o1, o0);
        float d = Vector3Length(dVec);
        Vector3 D = (d > 1e-4f) ? Vector3Scale(dVec, 1.0f / d) : Vector3{ 0.0f, 1.0f, 0.0f };

        float ratio = std::clamp((r0 - r1) / std::max(d, 1e-3f), -0.99f, 0.99f);
        float h0 = r0 * ratio;
        Vector3 center0 = Vector3Add(o0, Vector3Scale(D, h0));
        float radius0 = sqrtf(std::max(0.01f, r0 * r0 - h0 * h0));

        Vector3 refUp = (fabsf(D.y) < 0.9f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
        Vector3 u0 = Vector3Normalize(Vector3CrossProduct(D, refUp));
        Vector3 v0 = Vector3CrossProduct(D, u0);

        result.touchingCircles[0] = { center0, radius0, D, u0, v0 };
    }

    // 2. Inner spheres (s_i, i = 1 .. n-2) touching circles (Section 4, Fig. 11)
    for (size_t i = 1; i < n - 1; ++i) {
        Vector3 oPrev = spheres[i - 1].center;
        Vector3 oCurr = spheres[i].center;
        Vector3 oNext = spheres[i + 1].center;
        float rPrev = spheres[i - 1].radius;
        float rCurr = spheres[i].radius;
        float rNext = spheres[i + 1].radius;

        Vector3 vIn = Vector3Subtract(oCurr, oPrev);
        Vector3 vOut = Vector3Subtract(oNext, oCurr);
        Vector3 planeNormal = Vector3CrossProduct(vIn, vOut);
        float planeNormalLen = Vector3Length(planeNormal);

        Vector3 circCenter = oCurr;
        float circRadius = rCurr;
        Vector3 circNormal = Vector3Normalize(Vector3Add(vIn, vOut));

        if (planeNormalLen > 1e-4f) {
            Vector3 N_P = Vector3Scale(planeNormal, 1.0f / planeNormalLen);
            Vector3 xAxis = Vector3Normalize(vIn);
            Vector3 yAxis = Vector3CrossProduct(N_P, xAxis);

            // 2D projection on plane P_i with origin at oCurr
            std::vector<CircleData> triplet2D = {
                { { Vector3DotProduct(Vector3Subtract(oPrev, oCurr), xAxis), Vector3DotProduct(Vector3Subtract(oPrev, oCurr), yAxis) }, rPrev },
                { { 0.0f, 0.0f }, rCurr },
                { { Vector3DotProduct(Vector3Subtract(oNext, oCurr), xAxis), Vector3DotProduct(Vector3Subtract(oNext, oCurr), yAxis) }, rNext }
            };

            auto skin2D = computeSkinning(triplet2D, tangentScale, 16);
            if (skin2D.isValid && skin2D.leftPoints.size() >= 2 && skin2D.rightPoints.size() >= 2) {
                Vector2 pL = skin2D.leftPoints[1].point;
                Vector2 pR = skin2D.rightPoints[1].point;

                Vector3 pL3D = Vector3Add(oCurr, Vector3Add(Vector3Scale(xAxis, pL.x), Vector3Scale(yAxis, pL.y)));
                Vector3 pR3D = Vector3Add(oCurr, Vector3Add(Vector3Scale(xAxis, pR.x), Vector3Scale(yAxis, pR.y)));

                Vector3 chord = Vector3Subtract(pR3D, pL3D);
                float chordLen = Vector3Length(chord);
                if (chordLen > 1e-3f) {
                    circCenter = Vector3Scale(Vector3Add(pL3D, pR3D), 0.5f);
                    circRadius = 0.5f * chordLen;
                    circNormal = Vector3Normalize(Vector3CrossProduct(chord, N_P));
                    // Orient normal along curve progression
                    if (Vector3DotProduct(circNormal, Vector3Add(vIn, vOut)) < 0.0f) {
                        circNormal = Vector3Scale(circNormal, -1.0f);
                    }
                }
            }
        }

        // Parallel transport basis from previous touching circle to prevent twist
        Vector3 uPrev = result.touchingCircles[i - 1].u;
        Vector3 uProj = Vector3Subtract(uPrev, Vector3Scale(circNormal, Vector3DotProduct(uPrev, circNormal)));
        Vector3 uCurr = (Vector3Length(uProj) > 1e-3f) ? Vector3Normalize(uProj) : Vector3Normalize(Vector3CrossProduct(circNormal, { 0, 1, 0 }));
        Vector3 vCurr = Vector3CrossProduct(circNormal, uCurr);

        result.touchingCircles[i] = { circCenter, circRadius, circNormal, uCurr, vCurr };
    }

    // 3. Last sphere (s_n-1) touching circle via tangent cone with s_n-2
    {
        size_t last = n - 1;
        Vector3 oPrev = spheres[last - 1].center;
        Vector3 oLast = spheres[last].center;
        float rPrev = spheres[last - 1].radius;
        float rLast = spheres[last].radius;

        Vector3 dVec = Vector3Subtract(oLast, oPrev);
        float d = Vector3Length(dVec);
        Vector3 D = (d > 1e-4f) ? Vector3Scale(dVec, 1.0f / d) : Vector3{ 0.0f, 1.0f, 0.0f };

        float ratio = std::clamp((rPrev - rLast) / std::max(d, 1e-3f), -0.99f, 0.99f);
        float hLast = rLast * ratio;
        Vector3 centerLast = Vector3Subtract(oLast, Vector3Scale(D, hLast));
        float radiusLast = sqrtf(std::max(0.01f, rLast * rLast - hLast * hLast));

        Vector3 uPrev = result.touchingCircles[last - 1].u;
        Vector3 uProj = Vector3Subtract(uPrev, Vector3Scale(D, Vector3DotProduct(uPrev, D)));
        Vector3 uLast = (Vector3Length(uProj) > 1e-3f) ? Vector3Normalize(uProj) : Vector3Normalize(Vector3CrossProduct(D, { 0, 1, 0 }));
        Vector3 vLast = Vector3CrossProduct(D, uLast);

        result.touchingCircles[last] = { centerLast, radiusLast, D, uLast, vLast };
    }

    // 4. Generate Hermite surface patches between touching circles (n - 1 patches)
    result.patches.resize(n - 1);
    for (size_t i = 0; i < n - 1; ++i) {
        auto& patch = result.patches[i];
        patch.phiSteps = M;
        patch.tSteps = K;

        const auto& cA = result.touchingCircles[i];
        const auto& cB = result.touchingCircles[i + 1];
        const auto& sphA = spheres[i];
        const auto& sphB = spheres[i + 1];

        Vector3 dVec = Vector3Subtract(sphB.center, sphA.center);
        float d = Vector3Length(dVec);
        Vector3 D = (d > 1e-4f) ? Vector3Scale(dVec, 1.0f / d) : Vector3{ 0.0f, 1.0f, 0.0f };

        // Radical plane between sphA and sphB
        float dist1 = 0.5f * (d + (sphA.radius * sphA.radius - sphB.radius * sphB.radius) / std::max(d, 1e-3f));
        Vector3 pRad = Vector3Add(sphA.center, Vector3Scale(D, dist1));

        Vector3 N_A = cA.normal;
        Vector3 N_B = cB.normal;

        // Synchronize phi angle between circles cA and cB using reference vector e (Section 4)
        Vector3 e = Vector3CrossProduct(N_A, N_B);
        if (Vector3Length(e) < 1e-4f) {
            Vector3 ref = (fabsf(N_A.y) < 0.9f) ? Vector3{ 0.0f, 1.0f, 0.0f } : Vector3{ 1.0f, 0.0f, 0.0f };
            e = Vector3Normalize(Vector3CrossProduct(N_A, ref));
        } else {
            e = Vector3Normalize(e);
        }

        Vector3 uA = Vector3Normalize(Vector3CrossProduct(e, N_A));
        Vector3 vA = e;
        Vector3 uB = Vector3Normalize(Vector3CrossProduct(e, N_B));
        Vector3 vB = e;

        float hA = Vector3DotProduct(Vector3Subtract(cA.center, sphA.center), N_A);
        float hB = Vector3DotProduct(Vector3Subtract(cB.center, sphB.center), N_B);

        // Allocate vertex grid: (K + 1) * (M + 1)
        patch.vertices.resize((K + 1) * (M + 1));

        // Evaluate surface positions
        for (int stepPhi = 0; stepPhi <= M; ++stepPhi) {
            float phi = (float)stepPhi * 2.0f * PI / (float)M;
            float cosP = cosf(phi);
            float sinP = sinf(phi);

            Vector3 dirCircA = Vector3Add(Vector3Scale(uA, cosP), Vector3Scale(vA, sinP));
            Vector3 zA = Vector3Add(cA.center, Vector3Scale(dirCircA, cA.radius));

            Vector3 dirCircB = Vector3Add(Vector3Scale(uB, cosP), Vector3Scale(vB, sinP));
            Vector3 zB = Vector3Add(cB.center, Vector3Scale(dirCircB, cB.radius));

            // Exact unit tangents to spheres sphA and sphB pointing along the tube
            Vector3 tanUnitA = Vector3Scale(Vector3Subtract(Vector3Scale(N_A, cA.radius), Vector3Scale(dirCircA, hA)), 1.0f / sphA.radius);
            Vector3 tanUnitB = Vector3Scale(Vector3Subtract(Vector3Scale(N_B, cB.radius), Vector3Scale(dirCircB, hB)), 1.0f / sphB.radius);

            // Length proportional to distance to radical plane
            float distRadA = fabsf(Vector3DotProduct(Vector3Subtract(zA, pRad), D));
            float distRadB = fabsf(Vector3DotProduct(Vector3Subtract(zB, pRad), D));

            Vector3 vA = Vector3Scale(tanUnitA, tangentScale * distRadA);
            Vector3 vB = Vector3Scale(tanUnitB, tangentScale * distRadB);

            for (int stepT = 0; stepT <= K; ++stepT) {
                float t = (float)stepT / (float)K;
                Vector3 pos = evaluateHermite3D(zA, zB, vA, vB, t);
                int idx = stepT * (M + 1) + stepPhi;
                patch.vertices[idx].position = pos;
            }
        }

        // Compute smooth normals using finite differences on the grid
        for (int stepT = 0; stepT <= K; ++stepT) {
            float tFrac = (float)stepT / (float)K;
            Vector3 spineCenter = Vector3Add(Vector3Scale(sphA.center, 1.0f - tFrac), Vector3Scale(sphB.center, tFrac));

            for (int stepPhi = 0; stepPhi <= M; ++stepPhi) {
                int idx = stepT * (M + 1) + stepPhi;

                int tNext = std::min(stepT + 1, K);
                int tPrev = std::max(stepT - 1, 0);
                int phiNext = (stepPhi < M) ? stepPhi + 1 : 1;
                int phiPrev = (stepPhi > 0) ? stepPhi - 1 : M - 1;

                Vector3 dPosT = Vector3Subtract(patch.vertices[tNext * (M + 1) + stepPhi].position, patch.vertices[tPrev * (M + 1) + stepPhi].position);
                Vector3 dPosPhi = Vector3Subtract(patch.vertices[stepT * (M + 1) + phiNext].position, patch.vertices[stepT * (M + 1) + phiPrev].position);

                Vector3 norm = Vector3CrossProduct(dPosPhi, dPosT);
                if (Vector3Length(norm) < 1e-4f) norm = { 0.0f, 1.0f, 0.0f };

                // Ensure normal points strictly outward from local tube spine
                Vector3 outward = Vector3Subtract(patch.vertices[idx].position, spineCenter);
                if (Vector3DotProduct(norm, outward) < 0.0f) {
                    norm = Vector3Scale(norm, -1.0f);
                }

                patch.vertices[idx].normal = Vector3Normalize(norm);
            }
        }

        // Build triangle indices for the quad grid
        for (int stepT = 0; stepT < K; ++stepT) {
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

    result.isValid = true;
    return result;
}

} // namespace core
