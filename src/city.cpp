#include "city.h"
#include <algorithm>
#include <cmath>

City::City() : m_villaLoaded(false), m_aptLoaded(false) {}

City::~City() {
    if (m_villaLoaded) UnloadModel(m_modelVilla);
    if (m_aptLoaded) UnloadModel(m_modelApt);
}

void City::Init() {
    // Villa model
    m_modelVilla = LoadModel("building_villa.obj");
    if (m_modelVilla.meshCount <= 0 || m_modelVilla.meshes == nullptr) {
        m_modelVilla = LoadModel("assets/building_villa.obj");
    }
    m_villaLoaded = (m_modelVilla.meshCount > 0 && m_modelVilla.meshes != nullptr);

    // Apartment model
    m_modelApt = LoadModel("building_apt.obj");
    if (m_modelApt.meshCount <= 0 || m_modelApt.meshes == nullptr) {
        m_modelApt = LoadModel("assets/building_apt.obj");
    }
    m_aptLoaded = (m_modelApt.meshCount > 0 && m_modelApt.meshes != nullptr);

    m_buildings.clear();
    m_ramps.clear();
    m_trees.clear();
    m_streetLights.clear();

    // 1. Place buildings along 4 City Blocks
    // Block North-West
    for (int x = -100; x <= -40; x += 30) {
        for (int z = 40; z <= 100; z += 30) {
            int type = ((x + z) % 2 == 0) ? 0 : 1;
            m_buildings.push_back({ Vector3{ (float)x, 0.0f, (float)z }, 0.0f, 1.2f, type });
        }
    }
    // Block North-East
    for (int x = 40; x <= 100; x += 30) {
        for (int z = 40; z <= 100; z += 30) {
            int type = ((x + z) % 2 == 0) ? 1 : 0;
            m_buildings.push_back({ Vector3{ (float)x, 0.0f, (float)z }, 90.0f, 1.2f, type });
        }
    }
    // Block South-West
    for (int x = -100; x <= -40; x += 30) {
        for (int z = -100; z <= -40; z += 30) {
            int type = ((x + z) % 2 == 0) ? 0 : 1;
            m_buildings.push_back({ Vector3{ (float)x, 0.0f, (float)z }, 180.0f, 1.2f, type });
        }
    }
    // Block South-East
    for (int x = 40; x <= 100; x += 30) {
        for (int z = -100; z <= -40; z += 30) {
            int type = ((x + z) % 2 == 0) ? 1 : 0;
            m_buildings.push_back({ Vector3{ (float)x, 0.0f, (float)z }, 270.0f, 1.2f, type });
        }
    }

    // 2. Stunt Ramps for Airborne Jumps
    m_ramps.push_back({ Vector3{ 0.0f, 0.0f, 60.0f }, 0.0f, Vector3{ 8.0f, 2.5f, 12.0f } });
    m_ramps.push_back({ Vector3{ 0.0f, 0.0f, -60.0f }, 180.0f, Vector3{ 8.0f, 2.5f, 12.0f } });
    m_ramps.push_back({ Vector3{ 60.0f, 0.0f, 0.0f }, 90.0f, Vector3{ 8.0f, 2.5f, 12.0f } });
    m_ramps.push_back({ Vector3{ -60.0f, 0.0f, 0.0f }, 270.0f, Vector3{ 8.0f, 2.5f, 12.0f } });

    // 3. Sidewalk Trees
    for (int x = -120; x <= 120; x += 20) {
        if (abs(x) > 15) {
            m_trees.push_back(Vector3{ (float)x, 0.0f, 22.0f });
            m_trees.push_back(Vector3{ (float)x, 0.0f, -22.0f });
        }
    }
    for (int z = -120; z <= 120; z += 20) {
        if (abs(z) > 15) {
            m_trees.push_back(Vector3{ 22.0f, 0.0f, (float)z });
            m_trees.push_back(Vector3{ -22.0f, 0.0f, (float)z });
        }
    }
}

float City::GetSurfaceHeight(Vector3 carPos) {
    // Check if on a stunt ramp
    for (const auto& ramp : m_ramps) {
        float dx = carPos.x - ramp.pos.x;
        float dz = carPos.z - ramp.pos.z;
        if (fabsf(dx) < ramp.size.x * 0.5f && fabsf(dz) < ramp.size.z * 0.5f) {
            // Slope along length
            float progress = (dz + ramp.size.z * 0.5f) / ramp.size.z;
            if (ramp.yaw == 180.0f) progress = 1.0f - progress;
            return progress * ramp.size.y;
        }
    }
    return 0.0f;
}

bool City::CheckCollision(Vector3 carPos, float carRadius, Vector3& pushOut) {
    pushOut = { 0.0f, 0.0f, 0.0f };

    // 1. Boundary Wall (280m x 280m)
    float maxDist = 135.0f;
    if (carPos.x > maxDist)  { pushOut.x = (maxDist - carPos.x); return true; }
    if (carPos.x < -maxDist) { pushOut.x = (-maxDist - carPos.x); return true; }
    if (carPos.z > maxDist)  { pushOut.z = (maxDist - carPos.z); return true; }
    if (carPos.z < -maxDist) { pushOut.z = (-maxDist - carPos.z); return true; }

    // 2. Building Bounding Boxes
    for (const auto& b : m_buildings) {
        float bSizeX = (b.type == 0) ? 9.0f : 7.0f;
        float bSizeZ = (b.type == 0) ? 10.0f : 7.0f;

        float dx = carPos.x - b.pos.x;
        float dz = carPos.z - b.pos.z;

        float minX = (bSizeX * 0.5f) + carRadius;
        float minZ = (bSizeZ * 0.5f) + carRadius;

        if (fabsf(dx) < minX && fabsf(dz) < minZ) {
            float overlapX = minX - fabsf(dx);
            float overlapZ = minZ - fabsf(dz);

            if (overlapX < overlapZ) {
                pushOut.x = (dx > 0.0f) ? overlapX : -overlapX;
            } else {
                pushOut.z = (dz > 0.0f) ? overlapZ : -overlapZ;
            }
            return true;
        }
    }
    return false;
}

void City::Draw3D() {
    // 1. Ground Grass
    DrawPlane(Vector3{ 0.0f, -0.05f, 0.0f }, Vector2{ 350.0f, 350.0f }, Color{ 40, 120, 50, 255 });

    // 2. Main Boulevard Roads (Asphalt Cross & Ring)
    // Main North-South Avenue
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 24.0f, 0.02f, 280.0f, Color{ 35, 38, 42, 255 });
    // Main East-West Boulevard
    DrawCube(Vector3{ 0.0f, 0.0f, 0.0f }, 280.0f, 0.02f, 24.0f, Color{ 35, 38, 42, 255 });

    // Central Drift Plaza
    DrawCylinder(Vector3{ 0.0f, 0.01f, 0.0f }, 28.0f, 28.0f, 0.02f, 24, Color{ 45, 48, 55, 255 });
    DrawCylinder(Vector3{ 0.0f, 0.02f, 0.0f }, 6.0f, 6.0f, 1.2f, 16, Color{ 180, 50, 40, 255 }); // Center monument

    // Road markings
    for (int z = -130; z <= 130; z += 10) {
        if (abs(z) > 15) {
            DrawCube(Vector3{ 0.0f, 0.02f, (float)z }, 0.4f, 0.02f, 4.0f, YELLOW);
        }
    }
    for (int x = -130; x <= 130; x += 10) {
        if (abs(x) > 15) {
            DrawCube(Vector3{ (float)x, 0.02f, 0.0f }, 4.0f, 0.02f, 0.4f, YELLOW);
        }
    }

    // Outer Perimeter Barriers
    DrawCube(Vector3{ 0.0f, 1.0f, 138.0f }, 280.0f, 2.0f, 1.0f, Color{ 120, 125, 135, 255 });
    DrawCube(Vector3{ 0.0f, 1.0f, -138.0f }, 280.0f, 2.0f, 1.0f, Color{ 120, 125, 135, 255 });
    DrawCube(Vector3{ 138.0f, 1.0f, 0.0f }, 1.0f, 2.0f, 280.0f, Color{ 120, 125, 135, 255 });
    DrawCube(Vector3{ -138.0f, 1.0f, 0.0f }, 1.0f, 2.0f, 280.0f, Color{ 120, 125, 135, 255 });

    // 3. Render Stunt Ramps
    for (const auto& ramp : m_ramps) {
        DrawCube(Vector3{ ramp.pos.x, ramp.size.y * 0.5f, ramp.pos.z }, ramp.size.x, ramp.size.y, ramp.size.z, Color{ 220, 120, 20, 255 });
        DrawCubeWires(Vector3{ ramp.pos.x, ramp.size.y * 0.5f, ramp.pos.z }, ramp.size.x, ramp.size.y, ramp.size.z, WHITE);
    }

    // 4. Render 3D Buildings
    for (const auto& b : m_buildings) {
        if (b.type == 0 && m_villaLoaded) {
            DrawModelEx(m_modelVilla, b.pos, Vector3{ 0.0f, 1.0f, 0.0f }, b.yaw, Vector3{ b.scale, b.scale, b.scale }, WHITE);
        } else if (b.type == 1 && m_aptLoaded) {
            DrawModelEx(m_modelApt, b.pos, Vector3{ 0.0f, 1.0f, 0.0f }, b.yaw, Vector3{ b.scale, b.scale, b.scale }, WHITE);
        } else {
            // Procedural building fallback
            Vector3 size = (b.type == 0) ? Vector3{ 8.0f, 6.0f, 9.0f } : Vector3{ 7.0f, 12.0f, 7.0f };
            DrawCube(Vector3{ b.pos.x, size.y * 0.5f, b.pos.z }, size.x, size.y, size.z, (b.type == 0) ? Color{ 210, 190, 170, 255 } : Color{ 130, 140, 155, 255 });
            DrawCubeWires(Vector3{ b.pos.x, size.y * 0.5f, b.pos.z }, size.x, size.y, size.z, DARKGRAY);
        }
    }

    // 5. Trees
    for (const auto& pos : m_trees) {
        DrawCylinder(pos, 0.4f, 0.5f, 2.0f, 6, Color{ 100, 60, 25, 255 });
        DrawCylinder(Vector3{ pos.x, 2.0f, pos.z }, 2.0f, 0.8f, 2.2f, 7, Color{ 30, 120, 40, 255 });
        DrawCylinder(Vector3{ pos.x, 3.8f, pos.z }, 1.3f, 0.2f, 1.8f, 7, Color{ 40, 150, 50, 255 });
    }
}
