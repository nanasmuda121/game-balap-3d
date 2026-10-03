#include "bot.h"
#include <cmath>
#include <algorithm>

Bot::Bot() : m_skillLevel(1.0f), m_targetOffset(0.0f) {}
Bot::~Bot() {}

void Bot::Init(Vector3 startPos, float startYaw, const char* modelPath, Color color) {
    m_car.Init(startPos, startYaw, modelPath, color, false);
    m_skillLevel = 0.95f; // Well-matched competitor
    m_targetOffset = 0.0f;
}

void Bot::Reset(Vector3 startPos, float startYaw) {
    m_car.Reset(startPos, startYaw);
    m_targetOffset = 0.0f;
}

void Bot::Update(float dt, const Track& track, Vector3 playerPos) {
    Vector3 botPos = m_car.GetPosition();
    int currentIdx = track.GetClosestIndex(botPos);

    // 1. Dynamic look-ahead point on track
    float speed = m_car.GetSpeed();
    float lookAhead = 12.0f + speed * 0.75f;
    Vector3 targetPos = track.GetPointAhead(currentIdx, lookAhead);

    // 2. Obstacle & Player Avoidance
    Vector3 toPlayer = Vector3Subtract(playerPos, botPos);
    float distToPlayer = Vector3Length(toPlayer);
    Vector3 forward = m_car.GetForward();

    if (distToPlayer < 10.0f && Vector3DotProduct(forward, toPlayer) > 0.0f) {
        // Player is directly in front, choose side to overtake
        Vector3 right = Vector3{ -forward.z, 0.0f, forward.x };
        float sideDot = Vector3DotProduct(right, toPlayer);
        m_targetOffset = (sideDot > 0.0f) ? -3.5f : 3.5f; // Steer towards open lane
    } else {
        // Return to center racing line smoothly
        m_targetOffset = Lerp(m_targetOffset, 0.0f, 2.0f * dt);
    }

    const auto& pt = track.GetPoints()[currentIdx];
    targetPos = Vector3Add(targetPos, Vector3Scale(pt.right, m_targetOffset));

    // 3. Compute Steering Angle to Target
    Vector3 toTarget = Vector3Subtract(targetPos, botPos);
    float targetYaw = atan2f(-toTarget.x, -toTarget.z);

    float angleDiff = targetYaw - m_car.GetYaw();
    while (angleDiff > PI) angleDiff -= 2.0f * PI;
    while (angleDiff < -PI) angleDiff += 2.0f * PI;

    float steer = std::max(-1.0f, std::min(1.0f, angleDiff * 2.8f));

    // 4. Throttle and Braking Strategy
    float throttle = 1.0f;
    bool brake = false;
    bool nitro = false;

    // Check upcoming track curvature
    float curve = pt.curvature;
    if (fabsf(angleDiff) > 0.45f || curve > 0.12f) {
        // Approaching sharp corner, slow down
        if (speed > 22.0f * m_skillLevel) {
            brake = true;
            throttle = 0.2f;
        } else {
            throttle = 0.6f;
        }
    } else if (fabsf(angleDiff) < 0.12f && speed > 26.0f) {
        // Long straightaway: engage nitro!
        throttle = 1.0f;
        if (m_car.GetNitro() > 40.0f) {
            nitro = true;
        }
    }

    // 5. Update internal car
    m_car.Update(dt, throttle, steer, brake, false, nitro);

    // Update lap status
    float trackProgress = track.GetProgressAlongTrack(botPos);
    m_car.UpdateLapProgress(trackProgress, track.GetTotalLength());
}

void Bot::Draw3D() {
    m_car.Draw3D();
}
