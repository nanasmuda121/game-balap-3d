#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>

// Window and world constants
constexpr int TARGET_FPS = 60;
constexpr float TRACK_WIDTH = 14.0f;     // 14 meters wide road
constexpr float BARRIER_OFFSET = 8.5f;   // Crash barrier distance from center
constexpr int TOTAL_LAPS = 3;

enum GameState {
    STATE_COUNTDOWN,
    STATE_RACING,
    STATE_FINISHED,
    STATE_PAUSED
};

struct TrackPoint {
    Vector3 pos;        // Center of track at this point
    Vector3 forward;    // Unit vector along the track direction
    Vector3 right;      // Unit vector perpendicular to track (pointing right)
    float distance;     // Cumulative distance from start line along track
    float curvature;    // Measure of curvature (for AI braking)
};

// Helper vector math
inline Vector3 Vec3(float x, float y, float z) { return Vector3{ x, y, z }; }
inline Vector2 Vec2(float x, float y) { return Vector2{ x, y }; }
