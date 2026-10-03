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
      m_velY(0.0f),
      m_maxSpeed(38.0f),          // ~137 km/h
      m_accel(17.0f),             // m/s^2
      m_brakeForce(32.0f),        // m/s^2 (snappy braking)
      m_reverseMaxSpeed(-14.0f),  // ~-50 km/h
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
    m_velY = 0.0f;
    m_primaryColor = primaryColor;
    m_isPlayer = isPlayer;
    m_nitro = 100.0f;
    m_currentLap = 1;
    m_lapTime = 0.0f;
    m_bestLapTime = 999.0f;
    m_finished = false;
    m_lastPassedQuadrant = 0;

    if (m_modelLoaded) {
        UnloadModel(m_model);
        m_modelLoaded = false;
    }

    if (modelPath != nullptr) {
        // Strip "assets/" prefix if present to support direct Android APK assets root
        const char* cleanName = modelPath;
        if (strncmp(cleanName, "assets/", 7) == 0) {
            cleanName += 7;
        }

        // Try direct name first (standard for Android APK assets root)
        m_model = LoadModel(cleanName);
        if (IsModelReady(m_model) && m_model.meshCount > 0) {
            m_modelLoaded = true;
            TraceLog(LOG_INFO, "CAR: Successfully loaded model '%s'", cleanName);
        } else {
            // Try with "assets/" (standard for PC/desktop builds)
            const char* withAssets = TextFormat("assets/%s", cleanName);
            m_model = LoadModel(withAssets);
            if (IsModelReady(m_model) && m_model.meshCount > 0) {
                m_modelLoaded = true;
                TraceLog(LOG_INFO, "CAR: Successfully loaded model '%s'", withAssets);
            } else {
                TraceLog(LOG_WARNING, "CAR: Failed to load model '%s'", modelPath);
            }
        }
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

void Car::Update(float dt, float throttle, float steer, bool brake, bool reverse, bool nitro, float groundHeight) {
    if (m_finished) {
        throttle = 0.0f;
        brake = true;
        reverse = false;
        nitro = false;
    }

    UpdatePhysics(dt, throttle, steer, brake, reverse, nitro, groundHeight);
    UpdateParticles(dt);

    if (!m_finished) {
        m_lapTime += dt;
    }
}

void Car::UpdatePhysics(float dt, float throttle, float steer, bool brake, bool reverse, bool nitro, float groundHeight) {
    // 1. Nitro Boost
    m_isNitroActive = false;
    float currentTopSpeed = m_maxSpeed;
    float currentAccel = m_accel;

    if (nitro && m_nitro > 0.0f && throttle > 0.1f) {
        m_isNitroActive = true;
        currentTopSpeed *= 1.38f;   // ~190 km/h
        currentAccel *= 1.85f;
        m_nitro = std::max(0.0f, m_nitro - 28.0f * dt);
    } else {
        m_nitro = std::min(100.0f, m_nitro + 6.0f * dt);
    }

    // 2. Acceleration, Braking, and Reverse
    if (brake) {
        // Brake ONLY reduces speed towards 0 (NEVER accelerates backwards!)
        if (m_speed > 0.0f) {
            m_speed = std::max(0.0f, m_speed - m_brakeForce * dt);
        } else if (m_speed < 0.0f) {
            m_speed = std::min(0.0f, m_speed + m_brakeForce * dt);
        }
    } else if (reverse) {
        // Dedicated Reverse pedal
        if (m_speed > 0.5f) {
            m_speed = std::max(0.0f, m_speed - m_brakeForce * dt);
        } else {
            m_speed = std::max(m_reverseMaxSpeed, m_speed - m_accel * 0.7f * dt);
        }
    } else if (throttle > 0.01f) {
        // Forward Acceleration
        if (m_speed < 0.0f) {
            m_speed = std::min(0.0f, m_speed + m_brakeForce * dt);
        } else if (m_speed < currentTopSpeed) {
            m_speed += throttle * currentAccel * dt;
            if (m_speed > currentTopSpeed) m_speed = currentTopSpeed;
        }
    } else {
        // Rolling friction stops car smoothly at 0 (never negative!)
        float friction = 8.0f * dt;
        if (m_speed > friction) m_speed -= friction;
        else if (m_speed < -friction) m_speed += friction;
        else m_speed = 0.0f;
    }

    // 3. Air Drag
    m_speed -= 0.0007f * (m_speed * fabsf(m_speed)) * dt;

    // 4. Corrected Steering & Yaw Rotation
    float speedRatio = fabsf(m_speed) / m_maxSpeed;
    float turnFactor = (speedRatio > 0.75f) ? (1.6f - speedRatio * 0.6f) : (speedRatio * 1.6f);
    if (fabsf(m_speed) < 0.8f) turnFactor = fabsf(m_speed) * 0.8f; // Less steering when stationary

    float turnRate = 2.6f; // Radians per sec
    float steerDir = steer;
    if (m_speed < -0.1f) steerDir = -steer; // Inverted in reverse

    // steer < 0 (LEFT) increases yaw -> turns LEFT (-X)
    // steer > 0 (RIGHT) decreases yaw -> turns RIGHT (+X)
    m_yaw -= steerDir * turnRate * turnFactor * dt;

    // 5. Drift Detection
    m_isDrifting = (fabsf(steer) > 0.45f && fabsf(m_speed) > 16.0f);

    // 6. Velocity and Horizontal Position
    Vector3 forward = GetForward();
    m_vel = Vector3Scale(forward, m_speed);
    m_pos.x += m_vel.x * dt;
    m_pos.z += m_vel.z * dt;

    // 7. Vertical Position & Gravity (Stunt Jumps)
    if (m_pos.y > groundHeight + 0.05f) {
        m_velY -= 18.0f * dt; // Gravity
        m_pos.y += m_velY * dt;
        if (m_pos.y < groundHeight) {
            m_pos.y = groundHeight;
            m_velY = 0.0f;
        }
    } else {
        m_pos.y = groundHeight;
        m_velY = 0.0f;
    }

    // Dynamic Lean
    float targetRoll = steer * speedRatio * 0.08f;
    m_roll = Lerp(m_roll, targetRoll, 10.0f * dt);

    float targetPitch = (throttle - (brake ? 1.4f : 0.0f)) * 0.03f;
    m_pitch = Lerp(m_pitch, targetPitch, 8.0f * dt);
}

void Car::UpdateParticles(float dt) {
    Vector3 forward = GetForward();
    Vector3 right = Vector3{ -forward.z, 0.0f, forward.x };

    // Tire smoke during drift
    if (m_isDrifting && fabsf(m_speed) > 8.0f) {
        for (int k = 0; k < 2; k++) {
            float side = (k == 0) ? -1.0f : 1.0f;
            Vector3 tirePos = Vector3Add(m_pos, Vector3Add(Vector3Scale(right, side * 0.95f), Vector3Scale(forward, -1.8f)));
            tirePos.y = m_pos.y + 0.1f;
            m_particles.push_back({
                tirePos,
                Vector3{ (float)GetRandomValue(-2, 2) * 0.2f, (float)GetRandomValue(5, 15) * 0.1f, (float)GetRandomValue(-2, 2) * 0.2f },
                Color{ 220, 220, 220, 160 },
                0.45f,
                0.0f,
                0.55f
            });
        }
    }

    // Nitro exhaust fire
    if (m_isNitroActive) {
        for (int k = 0; k < 2; k++) {
            float side = (k == 0) ? -0.4f : 0.4f;
            Vector3 exhaustPos = Vector3Add(m_pos, Vector3Add(Vector3Scale(right, side), Vector3Scale(forward, -2.4f)));
            exhaustPos.y = m_pos.y + 0.35f;
            m_particles.push_back({
                exhaustPos,
                Vector3Add(Vector3Scale(forward, -m_speed * 0.45f), Vector3{ 0.0f, (float)GetRandomValue(0, 5) * 0.1f, 0.0f }),
                (GetRandomValue(0, 1) == 0) ? Color{ 0, 230, 255, 240 } : Color{ 60, 120, 255, 210 },
                0.35f,
                0.0f,
                0.28f
            });
        }
    }

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
    m_speed *= 0.65f;
}

void Car::Reset(Vector3 pos, float yaw) {
    m_pos = pos;
    m_yaw = yaw;
    m_speed = 0.0f;
    m_vel = { 0.0f, 0.0f, 0.0f };
    m_velY = 0.0f;
}

void Car::UpdateLapProgress(float trackProgress, float trackLength) {
    if (m_finished) return;

    int currentQuadrant = (int)((trackProgress / trackLength) * 4.0f) % 4;

    if (m_lastPassedQuadrant == 3 && currentQuadrant == 0) {
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
    for (const auto& p : m_particles) {
        float alpha = 1.0f - (p.life / p.maxLife);
        Color col = p.color;
        col.a = (unsigned char)(col.a * alpha);
        DrawSphere(p.pos, p.size, col);
    }

    if (m_modelLoaded) {
        // Draw centered, smoothly shaded 3D model
        DrawModelEx(
            m_model,
            m_pos,
            Vector3{ 0.0f, 1.0f, 0.0f },
            m_yaw * RAD2DEG,
            Vector3{ 1.0f, 1.0f, 1.0f },
            WHITE
        );
    } else {
        DrawProceduralCar();
    }
}

void Car::DrawProceduralCar() {
    Vector3 fwd = GetForward();
    Vector3 rgt = Vector3{ -fwd.z, 0.0f, fwd.x };

    Vector3 bodyPos = Vector3Add(m_pos, Vector3{ 0.0f, 0.5f, 0.0f });
    DrawCubeV(bodyPos, Vector3{ 2.2f, 0.6f, 4.4f }, m_primaryColor);
    DrawCubeWiresV(bodyPos, Vector3{ 2.2f, 0.6f, 4.4f }, DARKGRAY);

    Vector3 cabinPos = Vector3Add(m_pos, Vector3{ 0.0f, 1.0f, 0.2f });
    DrawCubeV(cabinPos, Vector3{ 1.7f, 0.65f, 2.2f }, Color{ 25, 25, 30, 255 });

    Vector3 spoilerPos = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -1.9f), Vector3{ 0.0f, 1.1f, 0.0f }));
    DrawCubeV(spoilerPos, Vector3{ 2.3f, 0.15f, 0.5f }, BLACK);

    Vector3 headL = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, 2.2f), Vector3Add(Vector3Scale(rgt, -0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    Vector3 headR = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, 2.2f), Vector3Add(Vector3Scale(rgt, 0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    DrawCube(headL, 0.4f, 0.25f, 0.2f, WHITE);
    DrawCube(headR, 0.4f, 0.25f, 0.2f, WHITE);

    Vector3 tailL = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -2.2f), Vector3Add(Vector3Scale(rgt, -0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    Vector3 tailR = Vector3Add(m_pos, Vector3Add(Vector3Scale(fwd, -2.2f), Vector3Add(Vector3Scale(rgt, 0.7f), Vector3{ 0.0f, 0.5f, 0.0f })));
    DrawCube(tailL, 0.4f, 0.25f, 0.2f, RED);
    DrawCube(tailR, 0.4f, 0.25f, 0.2f, RED);
}
