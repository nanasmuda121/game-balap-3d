#pragma once
#include "raylib.h"
#include <vector>
#include <cmath>

class AudioSystem {
public:
    AudioSystem();
    ~AudioSystem();

    void Init();
    void Close();

    void Update(float speedRatio, bool isDrifting, bool isNitro);
    void PlayCountdownBeep(bool isGo);
    void PlayCollision();
    void PlayNitro();

private:
    Sound m_sndBeep;
    Sound m_sndGo;
    Sound m_sndCrash;
    Sound m_sndNitro;
    Sound m_sndDrift;
    Sound m_sndEngine;

    bool m_initialized;
    float m_driftCooldown;
};
