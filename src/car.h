#pragma once
#include "common.h"
#include <vector>

struct Particle {
    Vector3 pos;
    Vector3 vel;
    Color color;
    float size;
    float life;
    float maxLife;
};

class Car {
public:
    Car();
    ~Car();

    void Init(Vector3 startPos, float startYaw, const char* modelPath, Color primaryColor, bool isPlayer);
    void Update(float dt, float throttle, float steer, bool brake, bool reverse, bool nitro, float groundHeight = 0.0f);
    void Draw3D();
    void ApplyCollisionImpulse(Vector3 impulse);
    void Reset(Vector3 pos, float yaw);

    // Getters
    Vector3 GetPosition() const { return m_pos; }
    Vector3 GetForward() const;
    float GetYaw() const { return m_yaw; }
    float GetSpeedKmh() const { return m_speed * 3.6f; }
    float GetSpeed() const { return m_speed; }
    float GetNitro() const { return m_nitro; }
    int GetLap() const { return m_currentLap; }
    float GetLapTime() const { return m_lapTime; }
    float GetBestLapTime() const { return m_bestLapTime; }
    bool HasFinished() const { return m_finished; }
    bool IsDrifting() const { return m_isDrifting; }
    bool IsNitroActive() const { return m_isNitroActive; }
    float GetRadius() const { return 1.6f; }

    // Lap progress update
    void UpdateLapProgress(float trackProgress, float trackLength);

private:
    void UpdatePhysics(float dt, float throttle, float steer, bool brake, bool reverse, bool nitro, float groundHeight);
    void UpdateParticles(float dt);
    void DrawProceduralCar();

    Vector3 m_pos;
    Vector3 m_vel;
    float m_yaw;          // Heading in radians
    float m_speed;        // Forward speed in m/s
    float m_steerAngle;   // Current steering wheel angle
    float m_roll;         // Body roll during turn
    float m_pitch;        // Squat on accelerate / dive on brake
    float m_velY;         // Vertical velocity for airborne jumps

    // Attributes
    float m_maxSpeed;
    float m_accel;
    float m_brakeForce;
    float m_reverseMaxSpeed;
    float m_nitro;        // 0.0 to 100.0
    bool m_isDrifting;
    bool m_isNitroActive;

    // Lap & Racing
    int m_currentLap;
    int m_lastPassedQuadrant;
    float m_lapTime;
    float m_bestLapTime;
    bool m_finished;

    // Visuals
    Model m_model;
    bool m_modelLoaded;
    Color m_primaryColor;
    bool m_isPlayer;

    // Particle system (drift smoke & nitro flame)
    std::vector<Particle> m_particles;
};
