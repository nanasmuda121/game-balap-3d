#pragma once
#include "car.h"
#include "track.h"

class Bot {
public:
    Bot();
    ~Bot();

    void Init(Vector3 startPos, float startYaw, const char* modelPath, Color color);
    void Reset(Vector3 startPos, float startYaw);
    void Update(float dt, const Track& track, Vector3 playerPos);
    void Draw3D();

    Car& GetCar() { return m_car; }
    const Car& GetCar() const { return m_car; }

private:
    Car m_car;
    float m_skillLevel;   // 0.8 to 1.1 for difficulty tuning
    float m_targetOffset; // Overtaking lateral bias (-1.0 to 1.0)
};
