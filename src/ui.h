#pragma once
#include "common.h"
#include "car.h"
#include "bot.h"
#include "track.h"

struct TouchInputState {
    float throttle;
    float steer;
    bool brake;
    bool nitro;
    bool reset;
};

class UI {
public:
    UI();
    ~UI();

    void Init();
    TouchInputState ProcessInput(bool isMobile);
    void DrawHUD(const Car& player, const Bot& bot, const Track& track, GameState state, float countdownTimer);
    void DrawCountdown(float countdownTimer);
    void DrawResults(bool playerWon, float totalTime, float bestLap, bool& outRestart);

private:
    void DrawSpeedometer(float speedKmh, float nitro, int screenW, int screenH);
    void DrawRaceStatus(int lap, int position, float lapTime, float bestLap, int screenW, int screenH);
    void DrawTouchControls(int screenW, int screenH, const TouchInputState& input);

    // Touch button bounding boxes
    Rectangle m_btnLeft;
    Rectangle m_btnRight;
    Rectangle m_btnGas;
    Rectangle m_btnBrake;
    Rectangle m_btnNitro;
    Rectangle m_btnReset;
};
