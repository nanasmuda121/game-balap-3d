#include "car.h"
#include <algorithm>
#include <cmath>

Car::Car() 
    : m_pos{ 0.0f, 0.0f, 0.0f },
      m_vel{ 0.0f, 0.0f, 0.0f },
      m_yaw(0.0f),
      m_speed(0.0f),
      m_steerAngle(0.0f),
      m_roll(0.0f),
      m_pitch(0.0f),
      m_maxSpeed(38.0f),          // ~137 km/h
      m_accel(16.0f),             // m/s^2
      m_brakeForce(28.0f),        // m/s^2
      m_reverseMaxSpeed(-12.0f),  // ~-43 km/h
      m_nitro(100.0f),
      m_isDrifting(false),
      m_isNitroActive(false),
      m_currentLap(1),
      m_lastPassedQuadrant(0),
      m_lapTime(0.0f),
      m_bestLapTime(999.0f),
      m_finished(false),
      m_modelLoaded(false),
      m_primaryColor(BLUE),
      m_isPlayer(true)
{
}

Car::~Car() {
    if (m_modelLoaded) {
        UnloadModel(m_model);
        m_modelLoaded = false;
    }
}

void Car::Init(Vector3 startPos, float startYaw, const char* modelPath, Color primaryColor, bool isPlayer) {
    m_pos = startPos;
    m_yaw = startYaw;
    m_speed = 0.0f;
    m_vel = { 0.0f, 0.0f, 0.0f };
    m_primaryColor = primaryColor;
    m_isPlayer = isPlayer;
    m_nitro = 100.0f;
    m_currentLap = 1;
    m_lapTime = 0.0f;
    m_bestLapTime = 999.0f;
    m_finished = false;
    m_lastPassedQuadrant = 0;

    if (modelPath != nullptr && FileExists(modelPath)) {
        m_model = LoadModel(modelPath);
        m_modelLoaded = true;
    } else {
        m_modelLoaded = false;
    }
}

Vector3 Car::GetForward() const {
    // Car forward is along -Z when yaw = 0
    return Vector3{
        -sinf(m_yaw),
        0.0f,
        -cosf(m_yaw)
    };
}

void Car::Update(float dt, float throttle, float steer, bool brake, bool nitro) {
    if (m_finished) {
        // Slow down automatically when finished
        throttle = 0.0f;
        brake = true;
        nitro = false;
    }

    UpdatePhysics(dt, throttle, steer, brake, nitro);
    UpdateParticles(dt);

    if (!m_finished) {
        m_lapTime += dt;
    }
}

void Car::UpdatePhysics(float dt, float throttle, float steer, bool brake, bool nitro) {
    // 1. Nitro Boost
    m_isNitroActive = false;
    float currentTopSpeed = m_maxSpeed;
    float currentAccel = m_accel;

    if (nitro && m_nitro > 0.0f && throttle > 0.1f) {
        m_isNitroActive = true;
        currentTopSpeed *= 1.35f;   // ~185 km/h
        currentAccel *= 1.8f;
        m_nitro = std::max(0.0f, m_nitro - 25.0f * dt);
    } else {
        // Slowly recharge nitro
        m_nitro = std::min(100.0f, m_nitro + 5.0f * dt);
    }

    // 2. Acceleration & Braking
    if (brake) {
        if (m_speed > 0.5f) {
            m_speed -= m_brakeForce * dt;
            if (m_speed < 0.0f) m_speed = 0.0f;
        } else {
            // Reverse
            m_speed -= m_accel * 0.5f * dt;
            if (m_speed < m_reverseMaxSpeed) m_speed = m_reverseMaxSpeed;
        }
    } else if (throttle > 0.01f) {
        if (m_speed < currentTopSpeed) {
            m_speed += throttle * currentAccel * dt;
            if (m_speed > currentTopSpeed) m_speed = currentTopSpeed;
        }
    } else {
        // Engine rolling friction
        float friction = 6.0f * dt;
        if (m_speed > friction) m_speed -= friction;
        else if (m_speed < -friction) m_speed += friction;
        else m_speed = 0.0f;
    }

    // 3. Air Drag
    m_speed -= 0.0008f * (m_speed * fabsf(m_speed)) * dt;

    // 4. Steering and Yaw
    float speedRatio = fabsf(m_speed) / m_maxSpeed;
    // Turn sensitivity is highest at mid speed, slightly less at top speed
    float turnFactor = (speedRatio > 0.7f) ? (1.7f - speedRatio) : (speedRatio * 1.5f);
    if (fabsf(m_speed) < 1.0f) turnFactor = fabsf(m_speed); // Don't turn when stopped

    float turnRate = 2.4f; // Radians per sec
    if (m_speed < 0.0f) steer = -steer; // Reverse steer

    m_yaw += steer * turnRate * turnFactor * dt;

    // 5. Drift Detection
    m_isDrifting = (fabsf(steer) > 0.5f && fabsf(m_speed) > 18.0f);

    // 6. Velocity and Position
    Vector3 forward = GetForward();
    m_vel = Vector3Scale(forward, m_speed);
    m_pos = Vector3Add(m_pos, Vector3Scale(m_vel, dt));

    // Dynamic Chassis Lean (Visual feedback)
    float targetRoll = -steer * speedRatio * 0.07f;
    m_roll = Lerp(m_roll, targetRoll, 10.0f * dt);

    float targetPitch = (throttle - (brake ? 1.2f : 0.0f)) * 0.03f;
    m_pitch = Lerp(m_pitch, targetPitch, 8.0f * dt);
}

void Car::UpdateParticles(float dt) {
    Vector3 forward = GetForward();
    Vector3 right = Vector3{ -forward.z, 0.0f, forward.x };

    // Spawn drift tire smoke
    if (m_isDrifting && fabsf(m_speed) > 10.0f) {
        for (int k = 0; k < 2; k++) {
            float side = (k == 0) ? -1.0f : 1.0f;
            Vector3 tirePos = Vector3Add(m_pos, Vector3Add(Vector3Scale(right, side * 0.9f), Vector3Scale(forward, -1.8f)));
            tirePos.y = 0.1f;
            m_particles.push_back({
                tirePos,
                Vector3{ (float)GetRandomValue(-2, 2) * 0.2f, (float)GetRandomValue(5, 15) * 0.1f, (float)GetRandomValue(-2, 2) * 0.2f },
                Color{ 220, 220, 220, 180 },
                0.4f,
                0.0f,
                0.5f
            });
        }
    }

    // Spawn nitro exhaust fire
    if (m_isNitroActive) {
        for (int k = 0; k < 2; k++) {
            float side = (k == 0) ? -0.4f : 0.4f;
            Vector3 exhaustPos = Vector3Add(m_pos, Vector3Add(Vector3Scale(right, side), Vector3Scale(forward, -2.4f)));
            exhaustPos.y = 0.35f;
            m_particles.push_back({
                exhaustPos,
                Vector3Add(Vector3Scale(forward, -m_speed * 0.4f), Vector3{ 0.0f, (float)GetRandomValue(0, 5) * 0.1f, 0.0f }),
                (GetRandomValue(0, 1) == 0) ? Color{ 0, 220, 255, 230 } : Color{ 50, 100, 255, 200 },
                0.3f,
                0.0f,
                0.25f
            });
        }
    }

    // Update existing particles
    for (size_t i = 0; i < m_particles.size();) {
        m_particles[i].life += dt;
        m_particles[i].pos = Vector3Add(m_particles[i].pos, Vector3Scale(m_particles[i].vel, dt));
        m_particles[i].size += dt * 0.5f;

        if (m_particles[i].life >= m_particles[i].maxLife) {
            m_particles[i] = m_particles.back();
            m_particles.pop_back();
        } else {
            i++;
        }
    }
}

void Car::ApplyCollisionImpulse(Vector3 impulse) {
    m_pos = Vector3Add(m_pos, impulse);
    m_speed *= 0.75f; // Lose speed on crash
}

void Car::Reset(Vector3 pos, float yaw) {
    m_pos = pos;
    m_yaw = yaw;
    m_speed = 0.0f;
    m_vel = { 0.0f, 0.0f, 0.0f };
}

void Car::UpdateLapProgress(float trackProgress, float trackLength) {
    if (m_finished) return;

    // Divide track into 4 quadrants to prevent reverse-cheating
    int currentQuadrant = (int)((trackProgress / trackLength) * 4.0f) % 4;

    if (m_lastPassedQuadrant == 3 && currentQuadrant == 0) {
        // Completed a valid lap!
        if (m_lapTime < m_bestLapTime) {
            m_bestLapTime = m_lapTime;
        }
        m_lapTime = 0.0f;
        m_currentLap++;

        if (m_currentLap > TOTAL_LAPS) {
            m_currentLap = TOTAL_LAPS;
            m_finished = true;
        }
    }
    m_lastPassedQuadrant = currentQuadrant;
}

void Car::Draw3D() {
    // 1. Draw Exhaust / Smoke Particles
    for (const auto& p : m_particles) {
        float alpha = 1.0f - (p.life / p.maxLife);
        Color col = p.color;
        col.a = (unsigned char)(col.a * alpha);
        DrawSphere(p.pos, p.size, col);
    }

    // 2. Render 3D Car Model
    if (m_modelLoaded) {
        // Draw the converted 3D model (mobil.3ma converted to OBJ)
        // Rotate along Y (yaw)
        DrawModelEx(
            m_model,
            m_pos,
            Vector3{ 0.0f, 1.0f, 0.0f },
            m_yaw * RAD2DEG,
            Vector3{ 1.0f, 1.0f, 1.0f },
            WHITE
        );
    } else {
        // High quality procedural 3D sports car fallback
        DrawProceduralCar();
    }
}

void Car::DrawProceduralCar() {
    Vector3 fwd = GetForward();
    Vector3 rgt = Vector3{ -fwd.z, 0.0f, fwd.x };

    // Lower Chassis
    Vector3 bodyPos = Vector3Add(m_pos, Vector3{ 0.0f, 0.5f, 0.0f });
    DrawCubeV(bodyPos, Vector3{ 2.2f, 0.6f, 4.4f }, m_primaryColor);
    DrawCubeWiresV(bodyPos, Vector3{ 2.2f, 0.6f, 4.4f }, DARKGRAY);

    // Cabin / Roof
    Vector3 cabinPos = Vector3Add(m_pos, Vector3{ 0.0f, 1.0f, 0.2f });
    DrawCubeV(cabinPos, Vector3{ 1.7f, 0.65f, 2.2f }, Color{ 25, 25, 30, 255 });

    // Spoiler
    Vector3 spoilerPos = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -1.9f), Vector3{ 0.0f, 1.1f, 0.0f }));
    DrawCubeV(spoilerPos, Vector3{ 2.3f, 0.15f, 0.5f }, BLACK);

    // Headlights (Front is along -Z / forward)
    Vector3 headL = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, 2.2f), Vector3Add(Vector3Scale(rgt, -0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    Vector3 headR = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, 2.2f), Vector3Add(Vector3Scale(rgt, 0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    DrawCube(headL, 0.4f, 0.25f, 0.2f, WHITE);
    DrawCube(headR, 0.4f, 0.25f, 0.2f, WHITE);

    // Taillights (Back is along +Z / -forward)
    Vector3 tailL = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -2.2f), Vector3Add(Vector3Scale(rgt, -0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    Vector3 tailR = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -2.2f), Vector3Add(Vector3Scale(rgt, 0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    DrawCube(tailL, 0.4f, 0.25f, 0.2f, RED);
    DrawCube(tailR, 0.4f, 0.25f, 0.2f, RED);

    // 4 Wheels
    float wheelOffsets[4][2] = {
        { -1.1f,  1.3f }, // Front-Left
        {  1.1f,  1.3f }, // Front-Right
        { -1.1f, -1.4f }, // Rear-Left
        {  1.1f, -1.4f }  // Rear-Right
    };

    for (int i = 0; i < 4; i++) {
        Vector3 wPos = Vector3Add(m_pos, Vector3Add(Vector3Scale(rgt, wheelOffsets[i][0]), Vector3Add(Vector3Scale(fwd, wheelOffsets[i][1]), Vector3{ 0.0f, 0.35f, 0.0f })));
        DrawCylinderEx(Vector3Add(wPos, Vector3Scale(rgt, -0.15f)), Vector3Add(wPos, Vector3Scale(rgt, 0.15f)), 0.4f, 0.4f, 10, BLACK);
    }
}
