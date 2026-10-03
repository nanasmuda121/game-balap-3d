#pragma once
#include "common.h"
#include "car.h"

class CameraFollow {
public:
    CameraFollow() {
        m_camera = { 0 };
        m_camera.position = Vector3{ 0.0f, 10.0f, 15.0f };
        m_camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
        m_camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
        m_camera.fovy = 52.0f;
        m_camera.projection = CAMERA_PERSPECTIVE;
        m_shake = 0.0f;
    }

    void Reset(const Car& car) {
        Vector3 carPos = car.GetPosition();
        Vector3 forward = car.GetForward();
        m_camera.position = Vector3Add(carPos, Vector3Add(Vector3Scale(forward, -7.5f), Vector3{ 0.0f, 3.2f, 0.0f }));
        m_camera.target = Vector3Add(carPos, Vector3Scale(forward, 4.0f));
    }

    void AddShake(float amount) {
        m_shake = std::min(1.0f, m_shake + amount);
    }

    void Update(float dt, const Car& car) {
        Vector3 carPos = car.GetPosition();
        Vector3 forward = car.GetForward();
        float speed = car.GetSpeed();

        // 1. Dynamic distance and FOV based on speed
        float speedRatio = std::min(1.0f, speed / 38.0f);
        float targetFov = 52.0f + speedRatio * 14.0f;
        m_camera.fovy = Lerp(m_camera.fovy, targetFov, 6.0f * dt);

        float followDist = 7.2f + speedRatio * 1.5f;
        float followHeight = 3.2f + speedRatio * 0.4f;

        // 2. Compute desired camera position behind the car
        Vector3 desiredPos = Vector3Add(carPos, Vector3Add(Vector3Scale(forward, -followDist), Vector3{ 0.0f, followHeight, 0.0f }));
        Vector3 desiredTarget = Vector3Add(carPos, Vector3Add(Vector3Scale(forward, 4.5f), Vector3{ 0.0f, 1.0f, 0.0f }));

        // 3. Screen shake (from drift, collisions, or nitro)
        if (m_shake > 0.001f) {
            float sx = (float)GetRandomValue(-10, 10) * 0.015f * m_shake;
            float sy = (float)GetRandomValue(-10, 10) * 0.015f * m_shake;
            desiredPos.x += sx;
            desiredPos.y += sy;
            m_shake = std::max(0.0f, m_shake - 3.0f * dt);
        }

        // 4. Smooth exponential camera interpolation
        float smoothPos = 10.0f * dt;
        float smoothTarget = 14.0f * dt;
        m_camera.position = Vector3Lerp(m_camera.position, desiredPos, std::min(1.0f, smoothPos));
        m_camera.target = Vector3Lerp(m_camera.target, desiredTarget, std::min(1.0f, smoothTarget));
    }

    const Camera3D& GetCamera() const { return m_camera; }

private:
    Camera3D m_camera;
    float m_shake;
};
