#include "track.h"
#include <algorithm>
#include <cmath>

static Vector3 CatmullRom(Vector3 p0, Vector3 p1, Vector3 p2, Vector3 p3, float t) {
    float t2 = t * t;
    float t3 = t2 * t;
    float x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
    float z = 0.5f * ((2.0f * p1.z) + (-p0.z + p2.z) * t + (2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 + (-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3);
    return Vector3{ x, 0.0f, z };
}

Track::Track() : m_totalLength(0.0f) {
    Init();
}

Track::~Track() {}

void Track::Init() {
    // 15 Key Grand Prix Waypoints (Closed Circuit Loop ~784m)
    m_baseWaypoints = {
        { 0.0f, 0.0f, -100.0f },
        { 0.0f, 0.0f, -50.0f },
        { 0.0f, 0.0f, 0.0f },      // Start / Finish Line
        { 0.0f, 0.0f, 50.0f },
        { 0.0f, 0.0f, 100.0f },
        { 25.0f, 0.0f, 140.0f },   // Turn 1 entry (fast right)
        { 70.0f, 0.0f, 160.0f },   // Turn 1 apex
        { 120.0f, 0.0f, 145.0f },  // Turn 1 exit
        { 155.0f, 0.0f, 110.0f },  // Turn 2
        { 165.0f, 0.0f, 50.0f },   // Back straightaway
        { 160.0f, 0.0f, -10.0f },
        { 145.0f, 0.0f, -70.0f },  // Hairpin entry
        { 115.0f, 0.0f, -120.0f },
        { 70.0f, 0.0f, -145.0f },  // Hairpin apex
        { 25.0f, 0.0f, -130.0f }   // Return sweeper onto main straight
    };

    GenerateSpline();
    GenerateDecorations();
}

void Track::GenerateSpline() {
    m_points.clear();
    int N = (int)m_baseWaypoints.size();
    constexpr int SUBDIVISIONS = 20;

    std::vector<Vector3> rawPositions;
    for (int i = 0; i < N; i++) {
        Vector3 p0 = m_baseWaypoints[(i - 1 + N) % N];
        Vector3 p1 = m_baseWaypoints[i];
        Vector3 p2 = m_baseWaypoints[(i + 1) % N];
        Vector3 p3 = m_baseWaypoints[(i + 2) % N];

        for (int s = 0; s < SUBDIVISIONS; s++) {
            float t = (float)s / (float)SUBDIVISIONS;
            rawPositions.push_back(CatmullRom(p0, p1, p2, p3, t));
        }
    }

    int totalPoints = (int)rawPositions.size();
    m_points.resize(totalPoints);
    m_totalLength = 0.0f;

    for (int i = 0; i < totalPoints; i++) {
        Vector3 curr = rawPositions[i];
        Vector3 next = rawPositions[(i + 1) % totalPoints];
        Vector3 prev = rawPositions[(i - 1 + totalPoints) % totalPoints];

        Vector3 forward = Vector3Normalize(Vector3Subtract(next, prev));
        Vector3 right = { -forward.z, 0.0f, forward.x }; // Perpendicular on XZ plane

        float stepDist = Vector3Distance(curr, next);

        // Curvature calculation
        Vector3 f1 = Vector3Normalize(Vector3Subtract(curr, prev));
        Vector3 f2 = Vector3Normalize(Vector3Subtract(next, curr));
        float dot = Vector3DotProduct(f1, f2);
        dot = std::max(-1.0f, std::min(1.0f, dot));
        float curvature = acosf(dot);

        m_points[i] = TrackPoint{
            curr,
            forward,
            right,
            m_totalLength,
            curvature
        };

        m_totalLength += stepDist;
    }
}

void Track::GenerateDecorations() {
    m_trees.clear();
    m_billboards.clear();
    m_lightPoles.clear();

    // Trees scattered around infield and outfield
    for (size_t i = 0; i < m_points.size(); i += 12) {
        const auto& pt = m_points[i];
        // Outfield tree
        m_trees.push_back({ Vector3Add(pt.pos, Vector3Scale(pt.right, 22.0f + (float)(i % 7) * 3.0f)), 1.0f + (float)(i % 5) * 0.15f });
        // Infield tree
        m_trees.push_back({ Vector3Add(pt.pos, Vector3Scale(pt.right, -24.0f - (float)((i+3) % 6) * 3.0f)), 0.9f + (float)(i % 4) * 0.2f });
    }

    // Light poles along track
    for (size_t i = 0; i < m_points.size(); i += 25) {
        const auto& pt = m_points[i];
        float angle = atan2f(pt.forward.x, pt.forward.z) * RAD2DEG;
        m_lightPoles.push_back({ Vector3Add(pt.pos, Vector3Scale(pt.right, 10.5f)), angle });
    }

    // Billboards on corner exits
    int billboardIndices[] = { 45, 95, 140, 190, 240, 280 };
    const char* messages[] = { "TURBO RACING", "SPEED NITRO", "APEX DRIFT", "CHAMPIONSHIP", "POLE POSITION", "MAX SPEED" };
    Color colors[] = { RED, BLUE, ORANGE, DARKPURPLE, GOLD, MAROON };

    for (int k = 0; k < 6; k++) {
        int idx = billboardIndices[k] % (int)m_points.size();
        const auto& pt = m_points[idx];
        float angle = atan2f(pt.forward.x, pt.forward.z) * RAD2DEG;
        m_billboards.push_back({
            Vector3Add(pt.pos, Vector3Scale(pt.right, 12.0f)),
            angle + 90.0f,
            messages[k],
            colors[k]
        });
    }
}

int Track::GetClosestIndex(Vector3 pos) const {
    int bestIdx = 0;
    float bestDistSq = 1e9f;
    for (size_t i = 0; i < m_points.size(); i++) {
        float dx = pos.x - m_points[i].pos.x;
        float dz = pos.z - m_points[i].pos.z;
        float d2 = dx * dx + dz * dz;
        if (d2 < bestDistSq) {
            bestDistSq = d2;
            bestIdx = (int)i;
        }
    }
    return bestIdx;
}

float Track::GetProgressAlongTrack(Vector3 pos) const {
    int idx = GetClosestIndex(pos);
    const auto& pt = m_points[idx];
    Vector3 toPos = Vector3Subtract(pos, pt.pos);
    float proj = Vector3DotProduct(toPos, pt.forward);
    return fmodf(pt.distance + proj + m_totalLength, m_totalLength);
}

Vector3 Track::GetPointAhead(int currentIndex, float lookAheadDistance) const {
    float targetDist = fmodf(m_points[currentIndex].distance + lookAheadDistance, m_totalLength);
    for (size_t i = 0; i < m_points.size(); i++) {
        if (m_points[i].distance >= targetDist) {
            return m_points[i].pos;
        }
    }
    return m_points[0].pos;
}

bool Track::CheckBarrierCollision(Vector3 carPos, float carRadius, Vector3& pushOutForce) {
    int idx = GetClosestIndex(carPos);
    const auto& pt = m_points[idx];

    Vector3 toCar = Vector3Subtract(carPos, pt.pos);
    float lateralDist = Vector3DotProduct(toCar, pt.right); // distance from centerline (+ right, - left)

    float maxAllowed = BARRIER_OFFSET - carRadius;
    if (fabsf(lateralDist) > maxAllowed) {
        float penetration = fabsf(lateralDist) - maxAllowed;
        float sign = (lateralDist > 0.0f) ? -1.0f : 1.0f;
        pushOutForce = Vector3Scale(pt.right, sign * penetration * 1.5f);
        return true;
    }
    pushOutForce = { 0.0f, 0.0f, 0.0f };
    return false;
}

void Track::Draw3D() {
    // 1. Vast Terrain Grass Plane
    DrawPlane(Vector3{ 80.0f, -0.05f, 0.0f }, Vector2{ 500.0f, 500.0f }, Color{ 34, 110, 48, 255 });

    int total = (int)m_points.size();
    float halfRoad = TRACK_WIDTH * 0.5f;
    float curbWidth = 1.4f;

    // 2. Render Road, Curbs, and Guardrails in segments
    for (int i = 0; i < total; i++) {
        const auto& p1 = m_points[i];
        const auto& p2 = m_points[(i + 1) % total];

        // Road vertices
        Vector3 rL1 = Vector3Add(p1.pos, Vector3Scale(p1.right, -halfRoad));
        Vector3 rR1 = Vector3Add(p1.pos, Vector3Scale(p1.right, halfRoad));
        Vector3 rL2 = Vector3Add(p2.pos, Vector3Scale(p2.right, -halfRoad));
        Vector3 rR2 = Vector3Add(p2.pos, Vector3Scale(p2.right, halfRoad));

        // Asphalt Road quad
        Color asphaltColor = ((i / 4) % 2 == 0) ? Color{ 40, 42, 46, 255 } : Color{ 45, 47, 51, 255 };
        DrawTriangle3D(rL1, rR1, rL2, asphaltColor);
        DrawTriangle3D(rR1, rR2, rL2, asphaltColor);

        // Center Dashed White Line
        if (i % 3 != 0) {
            Vector3 cL1 = Vector3Add(p1.pos, Vector3Scale(p1.right, -0.2f));
            Vector3 cR1 = Vector3Add(p1.pos, Vector3Scale(p1.right, 0.2f));
            Vector3 cL2 = Vector3Add(p2.pos, Vector3Scale(p2.right, -0.2f));
            Vector3 cR2 = Vector3Add(p2.pos, Vector3Scale(p2.right, 0.2f));
            cL1.y += 0.01f; cR1.y += 0.01f; cL2.y += 0.01f; cR2.y += 0.01f;
            DrawTriangle3D(cL1, cR1, cL2, WHITE);
            DrawTriangle3D(cR1, cR2, cL2, WHITE);
        }

        // Curbs (Red and White alternating)
        Color curbColor = ((i / 3) % 2 == 0) ? RED : RAYWHITE;

        // Left Curb
        Vector3 curbOutL1 = Vector3Add(rL1, Vector3Scale(p1.right, -curbWidth));
        Vector3 curbOutL2 = Vector3Add(rL2, Vector3Scale(p2.right, -curbWidth));
        curbOutL1.y += 0.04f; curbOutL2.y += 0.04f;
        DrawTriangle3D(curbOutL1, rL1, curbOutL2, curbColor);
        DrawTriangle3D(rL1, rL2, curbOutL2, curbColor);

        // Right Curb
        Vector3 curbOutR1 = Vector3Add(rR1, Vector3Scale(p1.right, curbWidth));
        Vector3 curbOutR2 = Vector3Add(rR2, Vector3Scale(p2.right, curbWidth));
        curbOutR1.y += 0.04f; curbOutR2.y += 0.04f;
        DrawTriangle3D(rR1, curbOutR1, rR2, curbColor);
        DrawTriangle3D(curbOutR1, curbOutR2, rR2, curbColor);

        // Armco Crash Barriers (Left & Right)
        Vector3 barL1 = Vector3Add(p1.pos, Vector3Scale(p1.right, -BARRIER_OFFSET));
        Vector3 barL2 = Vector3Add(p2.pos, Vector3Scale(p2.right, -BARRIER_OFFSET));
        Vector3 barR1 = Vector3Add(p1.pos, Vector3Scale(p1.right, BARRIER_OFFSET));
        Vector3 barR2 = Vector3Add(p2.pos, Vector3Scale(p2.right, BARRIER_OFFSET));

        // Left Barrier wall
        Vector3 barL1_top = { barL1.x, 1.1f, barL1.z };
        Vector3 barL2_top = { barL2.x, 1.1f, barL2.z };
        DrawTriangle3D(barL1, barL1_top, barL2, Color{ 140, 145, 155, 255 });
        DrawTriangle3D(barL1_top, barL2_top, barL2, Color{ 170, 175, 185, 255 });

        // Right Barrier wall
        Vector3 barR1_top = { barR1.x, 1.1f, barR1.z };
        Vector3 barR2_top = { barR2.x, 1.1f, barR2.z };
        DrawTriangle3D(barR1_top, barR1, barR2_top, Color{ 140, 145, 155, 255 });
        DrawTriangle3D(barR1, barR2, barR2_top, Color{ 170, 175, 185, 255 });

        // Barrier posts every 6 segments
        if (i % 6 == 0) {
            DrawCube(barL1_top, 0.3f, 1.2f, 0.3f, DARKGRAY);
            DrawCube(barR1_top, 0.3f, 1.2f, 0.3f, DARKGRAY);
        }
    }

    // 3. Start / Finish Line Checkered Banner & 3D Truss Arch
    int startIdx = 40; // Main straightaway start point
    const auto& sPt = m_points[startIdx];
    Vector3 sL = Vector3Add(sPt.pos, Vector3Scale(sPt.right, -halfRoad));
    Vector3 sR = Vector3Add(sPt.pos, Vector3Scale(sPt.right, halfRoad));

    // Checkered finish line pattern on track
    for (int b = 0; b < 14; b++) {
        float f0 = (float)b / 14.0f;
        float f1 = (float)(b + 1) / 14.0f;
        Vector3 pA = Vector3Lerp(sL, sR, f0);
        Vector3 pB = Vector3Lerp(sL, sR, f1);
        Color chk = (b % 2 == 0) ? WHITE : BLACK;
        DrawCube(Vector3Add(pA, Vector3{ 0.0f, 0.02f, 0.0f }), 1.0f, 0.02f, 2.0f, chk);
    }

    // 3D Start Arch Gantry
    Vector3 pillarL = Vector3Add(sPt.pos, Vector3Scale(sPt.right, -BARRIER_OFFSET - 0.5f));
    Vector3 pillarR = Vector3Add(sPt.pos, Vector3Scale(sPt.right, BARRIER_OFFSET + 0.5f));
    pillarL.y = 3.5f;
    pillarR.y = 3.5f;

    // Arch Pillars
    DrawCube(pillarL, 0.8f, 7.0f, 0.8f, Color{ 30, 30, 35, 255 });
    DrawCube(pillarR, 0.8f, 7.0f, 0.8f, Color{ 30, 30, 35, 255 });

    // Arch Top Crossbar & Banner
    Vector3 archCenter = Vector3Lerp(pillarL, pillarR, 0.5f);
    archCenter.y = 7.0f;
    DrawCube(archCenter, BARRIER_OFFSET * 2.2f, 1.8f, 1.0f, Color{ 220, 20, 20, 255 });
    DrawCubeWires(archCenter, BARRIER_OFFSET * 2.2f, 1.8f, 1.0f, GOLD);

    // 4. Grandstand (Tribun Penonton) on Main Straight
    Vector3 standCenter = { -24.0f, 2.5f, 0.0f };
    DrawCube(standCenter, 6.0f, 5.0f, 60.0f, Color{ 180, 185, 195, 255 });
    // Bleacher tiers
    DrawCube(Vector3{ -22.5f, 1.5f, 0.0f }, 2.5f, 3.0f, 58.0f, Color{ 20, 100, 220, 255 });
    DrawCube(Vector3{ -25.0f, 3.5f, 0.0f }, 2.5f, 5.0f, 58.0f, Color{ 220, 40, 30, 255 });
    // Grandstand Roof
    DrawCube(Vector3{ -22.0f, 6.5f, 0.0f }, 12.0f, 0.4f, 64.0f, Color{ 230, 230, 235, 255 });

    // 5. Render 3D Low-Poly Trees
    for (const auto& tree : m_trees) {
        // Brown Trunk
        DrawCylinder(Vector3{ tree.pos.x, 0.0f, tree.pos.z }, 0.5f * tree.scale, 0.6f * tree.scale, 2.5f * tree.scale, 6, Color{ 110, 65, 30, 255 });
        // Green Foliage (layered cones)
        DrawCylinder(Vector3{ tree.pos.x, 2.2f * tree.scale, tree.pos.z }, 2.6f * tree.scale, 1.2f * tree.scale, 2.4f * tree.scale, 7, Color{ 35, 130, 45, 255 });
        DrawCylinder(Vector3{ tree.pos.x, 4.0f * tree.scale, tree.pos.z }, 1.8f * tree.scale, 0.2f * tree.scale, 2.2f * tree.scale, 7, Color{ 45, 160, 55, 255 });
    }

    // 6. Render 3D Sponsor Billboards
    for (const auto& bb : m_billboards) {
        // Posts
        DrawCube(Vector3{ bb.pos.x, 2.0f, bb.pos.z }, 0.4f, 4.0f, 0.4f, DARKGRAY);
        // Billboard Board
        DrawCube(Vector3{ bb.pos.x, 4.0f, bb.pos.z }, 6.5f, 2.2f, 0.4f, bb.color);
        DrawCubeWires(Vector3{ bb.pos.x, 4.0f, bb.pos.z }, 6.5f, 2.2f, 0.4f, WHITE);
    }
}

void Track::DrawMiniMap(Rectangle bounds, Vector3 playerPos, Vector3 botPos, float playerYaw, float botYaw) {
    // Background card
    DrawRectangleRec(bounds, Color{ 20, 25, 35, 200 });
    DrawRectangleLinesEx(bounds, 2.0f, Color{ 60, 75, 100, 255 });
    DrawText("CIRCUIT RADAR", (int)bounds.x + 8, (int)bounds.y + 6, 12, Color{ 180, 200, 225, 255 });

    // Track bounds mapping: X from -30 to 180, Z from -160 to 180
    float minX = -35.0f, maxX = 185.0f;
    float minZ = -165.0f, maxZ = 185.0f;

    auto WorldToMap = [&](Vector3 w) -> Vector2 {
        float nx = (w.x - minX) / (maxX - minX);
        float ny = (w.z - minZ) / (maxZ - minZ);
        // Padding inside radar
        float px = bounds.x + 10.0f + nx * (bounds.width - 20.0f);
        float py = bounds.y + 20.0f + ny * (bounds.height - 30.0f);
        return Vector2{ px, py };
    };

    // Draw track loop lines
    int total = (int)m_points.size();
    for (int i = 0; i < total; i += 2) {
        Vector2 pA = WorldToMap(m_points[i].pos);
        Vector2 pB = WorldToMap(m_points[(i + 2) % total].pos);
        DrawLineEx(pA, pB, 4.0f, Color{ 60, 65, 80, 255 });
    }

    // Start/Finish indicator (White marker)
    Vector2 startMap = WorldToMap(m_points[40].pos);
    DrawCircleV(startMap, 4.0f, GOLD);

    // Bot Dot (Red)
    Vector2 botMap = WorldToMap(botPos);
    DrawCircleV(botMap, 5.0f, RED);
    DrawCircleLines((int)botMap.x, (int)botMap.y, 6.0f, MAROON);

    // Player Dot (Cyan/Blue with direction pulse)
    Vector2 playerMap = WorldToMap(playerPos);
    DrawCircleV(playerMap, 6.0f, Color{ 0, 220, 255, 255 });
    DrawCircleLines((int)playerMap.x, (int)playerMap.y, 7.0f, WHITE);
}
