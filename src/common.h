#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <string>
#include <cmath>

constexpr int TARGET_FPS = 60;
constexpr float TRACK_WIDTH = 14.0f;
constexpr float BARRIER_OFFSET = 8.5f;
constexpr int TOTAL_LAPS = 3;

enum GameMode {
    MODE_BALAPAN,
    MODE_OPENWORLD
};

enum GameState {
    STATE_MENU,
    STATE_COUNTDOWN,
    STATE_PLAYING,
    STATE_FINISHED
};

struct TrackPoint {
    Vector3 pos;
    Vector3 forward;
    Vector3 right;
    float distance;
    float curvature;
};

inline Vector3 Vec3(float x, float y, float z) { return Vector3{ x, y, z }; }
inline Vector2 Vec2(float x, float y) { return Vector2{ x, y }; }
