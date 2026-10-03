#include "ui.h"
#include <cstdio>
#include <algorithm>

UI::UI() {}
UI::~UI() {}

void UI::Init() {}

TouchInputState UI::ProcessInput(bool isMobile) {
    TouchInputState input = { 0.0f, 0.0f, false, false, false };
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // 1. Update Responsive Touch Button Boundaries
    float btnSize = screenH * 0.18f;
    if (btnSize < 75.0f) btnSize = 75.0f;
    if (btnSize > 120.0f) btnSize = 120.0f;

    m_btnLeft  = Rectangle{ 30.0f, screenH - btnSize - 30.0f, btnSize, btnSize };
    m_btnRight = Rectangle{ 45.0f + btnSize, screenH - btnSize - 30.0f, btnSize, btnSize };

    m_btnGas   = Rectangle{ screenW - btnSize - 30.0f, screenH - btnSize * 1.35f - 25.0f, btnSize, btnSize * 1.35f };
    m_btnBrake = Rectangle{ screenW - btnSize * 2.15f - 40.0f, screenH - btnSize - 25.0f, btnSize, btnSize };
    m_btnNitro = Rectangle{ screenW - btnSize * 2.15f - 40.0f, screenH - btnSize * 2.2f - 40.0f, btnSize, btnSize * 0.85f };

    m_btnReset = Rectangle{ screenW * 0.5f - 60.0f, 20.0f, 120.0f, 38.0f };

    // 2. Keyboard Inputs (PC / Terminal testing)
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.throttle = 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.brake = true;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.steer -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.steer += 1.0f;
    if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_SHIFT)) input.nitro = true;
    if (IsKeyPressed(KEY_R)) input.reset = true;

    // 3. Multi-Touch Screen Inputs (Android Touchscreen)
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);

        if (CheckCollisionPointRec(touch, m_btnLeft))  input.steer -= 1.0f;
        if (CheckCollisionPointRec(touch, m_btnRight)) input.steer += 1.0f;
        if (CheckCollisionPointRec(touch, m_btnGas))   input.throttle = 1.0f;
        if (CheckCollisionPointRec(touch, m_btnBrake)) input.brake = true;
        if (CheckCollisionPointRec(touch, m_btnNitro)) input.nitro = true;
        if (CheckCollisionPointRec(touch, m_btnReset)) input.reset = true;
    }

    // Also support mouse click simulation
    if (touchCount == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, m_btnLeft))  input.steer -= 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnRight)) input.steer += 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnGas))   input.throttle = 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnBrake)) input.brake = true;
        if (CheckCollisionPointRec(mouse, m_btnNitro)) input.nitro = true;
        if (CheckCollisionPointRec(mouse, m_btnReset)) input.reset = true;
    }

    input.steer = std::max(-1.0f, std::min(1.0f, input.steer));
    return input;
}

void UI::DrawHUD(const Car& player, const Bot& bot, const Track& track, GameState state, float countdownTimer) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // 1. Speedometer & Nitro (Bottom Center)
    DrawSpeedometer(player.GetSpeedKmh(), player.GetNitro(), screenW, screenH);

    // 2. Race Position & Lap Stats (Top Left)
    float playerDist = player.GetLap() * track.GetTotalLength() + track.GetProgressAlongTrack(player.GetPosition());
    float botDist = bot.GetCar().GetLap() * track.GetTotalLength() + track.GetProgressAlongTrack(bot.GetCar().GetPosition());
    int position = (playerDist >= botDist) ? 1 : 2;

    DrawRaceStatus(player.GetLap(), position, player.GetLapTime(), player.GetBestLapTime(), screenW, screenH);

    // 3. Mini-Map Radar (Top Right)
    float mapSize = std::min(screenW * 0.22f, 180.0f);
    Rectangle mapBounds = { screenW - mapSize - 20.0f, 20.0f, mapSize, mapSize };
    const_cast<Track&>(track).DrawMiniMap(mapBounds, player.GetPosition(), bot.GetCar().GetPosition(), player.GetYaw(), bot.GetCar().GetYaw());

    // 4. On-Screen Touch Controls (Mobile Friendly)
    TouchInputState dummyState = ProcessInput(true);
    DrawTouchControls(screenW, screenH, dummyState);

    // 5. Reset button on top
    DrawRectangleRounded(m_btnReset, 0.3f, 6, Color{ 40, 45, 60, 200 });
    DrawRectangleRoundedLinesEx(m_btnReset, 0.3f, 6, 2.0f, Color{ 100, 115, 145, 255 });
    DrawText("RESET [R]", (int)m_btnReset.x + 20, (int)m_btnReset.y + 11, 16, WHITE);
}

void UI::DrawSpeedometer(float speedKmh, float nitro, int screenW, int screenH) {
    int centerX = screenW / 2;
    int centerY = screenH - 55;

    // Background card
    Rectangle bg = { (float)centerX - 130.0f, (float)centerY - 45.0f, 260.0f, 90.0f };
    DrawRectangleRounded(bg, 0.3f, 6, Color{ 15, 18, 25, 220 });
    DrawRectangleRoundedLinesEx(bg, 0.3f, 6, 2.0f, Color{ 45, 55, 75, 255 });

    // Digital speed
    char speedText[32];
    snprintf(speedText, sizeof(speedText), "%3.0f", speedKmh);
    DrawText(speedText, centerX - 55, centerY - 38, 44, WHITE);
    DrawText("KM/H", centerX + 25, centerY - 20, 18, Color{ 160, 180, 210, 255 });

    // Nitro Bar
    DrawText("NITRO", centerX - 110, centerY + 18, 13, Color{ 0, 200, 255, 255 });
    Rectangle nitroBg = { (float)centerX - 55.0f, (float)centerY + 18.0f, 165.0f, 14.0f };
    DrawRectangleRec(nitroBg, Color{ 30, 35, 45, 255 });
    float fillW = (nitro / 100.0f) * 165.0f;
    DrawRectangleRec(Rectangle{ nitroBg.x, nitroBg.y, fillW, nitroBg.height }, Color{ 0, 220, 255, 255 });
    DrawRectangleLinesEx(nitroBg, 1.0f, Color{ 80, 100, 130, 255 });
}

void UI::DrawRaceStatus(int lap, int position, float lapTime, float bestLap, int screenW, int screenH) {
    // Position Badge (P1 / P2)
    Rectangle posBadge = { 20.0f, 20.0f, 85.0f, 85.0f };
    Color badgeColor = (position == 1) ? Color{ 0, 180, 255, 230 } : Color{ 230, 120, 20, 230 };
    DrawRectangleRounded(posBadge, 0.25f, 6, badgeColor);
    DrawRectangleRoundedLinesEx(posBadge, 0.25f, 6, 2.0f, WHITE);

    char posStr[8];
    snprintf(posStr, sizeof(posStr), "%dst", position);
    if (position == 2) snprintf(posStr, sizeof(posStr), "2nd");
    DrawText(posStr, (int)posBadge.x + 14, (int)posBadge.y + 24, 34, WHITE);
    DrawText("POS", (int)posBadge.x + 28, (int)posBadge.y + 64, 11, Color{ 220, 240, 255, 255 });

    // Lap Counter Box
    Rectangle lapBox = { 115.0f, 20.0f, 150.0f, 85.0f };
    DrawRectangleRounded(lapBox, 0.2f, 6, Color{ 20, 25, 35, 210 });
    DrawRectangleRoundedLinesEx(lapBox, 0.2f, 6, 2.0f, Color{ 55, 70, 95, 255 });

    char lapStr[32];
    snprintf(lapStr, sizeof(lapStr), "LAP %d / %d", lap, TOTAL_LAPS);
    DrawText(lapStr, (int)lapBox.x + 15, (int)lapBox.y + 12, 20, (lap == TOTAL_LAPS) ? GOLD : WHITE);

    // Lap Timer
    int mins = (int)lapTime / 60;
    float secs = lapTime - (mins * 60);
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%05.2f", mins, secs);
    DrawText(timeStr, (int)lapBox.x + 15, (int)lapBox.y + 38, 16, Color{ 0, 240, 180, 255 });

    if (bestLap < 900.0f) {
        int bMins = (int)bestLap / 60;
        float bSecs = bestLap - (bMins * 60);
        char bestStr[32];
        snprintf(bestStr, sizeof(bestStr), "BEST %02d:%05.2f", bMins, bSecs);
        DrawText(bestStr, (int)lapBox.x + 15, (int)lapBox.y + 60, 13, Color{ 180, 190, 205, 255 });
    }
}

void UI::DrawTouchControls(int screenW, int screenH, const TouchInputState& input) {
    // Steer Left Button
    Color colLeft = (input.steer < -0.1f) ? Color{ 0, 200, 255, 220 } : Color{ 40, 50, 70, 160 };
    DrawRectangleRounded(m_btnLeft, 0.4f, 6, colLeft);
    DrawRectangleRoundedLinesEx(m_btnLeft, 0.4f, 6, 2.5f, WHITE);
    DrawText("<", (int)(m_btnLeft.x + m_btnLeft.width * 0.38f), (int)(m_btnLeft.y + m_btnLeft.height * 0.22f), 48, WHITE);

    // Steer Right Button
    Color colRight = (input.steer > 0.1f) ? Color{ 0, 200, 255, 220 } : Color{ 40, 50, 70, 160 };
    DrawRectangleRounded(m_btnRight, 0.4f, 6, colRight);
    DrawRectangleRoundedLinesEx(m_btnRight, 0.4f, 6, 2.5f, WHITE);
    DrawText(">", (int)(m_btnRight.x + m_btnRight.width * 0.38f), (int)(m_btnRight.y + m_btnRight.height * 0.22f), 48, WHITE);

    // Gas Pedal (Green)
    Color colGas = (input.throttle > 0.1f) ? Color{ 40, 220, 80, 240 } : Color{ 25, 120, 50, 170 };
    DrawRectangleRounded(m_btnGas, 0.3f, 6, colGas);
    DrawRectangleRoundedLinesEx(m_btnGas, 0.3f, 6, 2.5f, WHITE);
    DrawText("GAS", (int)(m_btnGas.x + m_btnGas.width * 0.22f), (int)(m_btnGas.y + m_btnGas.height * 0.35f), 30, WHITE);

    // Brake / Reverse Pedal (Red)
    Color colBrake = input.brake ? Color{ 240, 50, 50, 240 } : Color{ 140, 30, 30, 170 };
    DrawRectangleRounded(m_btnBrake, 0.3f, 6, colBrake);
    DrawRectangleRoundedLinesEx(m_btnBrake, 0.3f, 6, 2.5f, WHITE);
    DrawText("BRAKE", (int)(m_btnBrake.x + m_btnBrake.width * 0.12f), (int)(m_btnBrake.y + m_btnBrake.height * 0.32f), 20, WHITE);

    // Nitro Button (Cyan)
    Color colNitro = input.nitro ? Color{ 0, 240, 255, 255 } : Color{ 0, 130, 190, 180 };
    DrawRectangleRounded(m_btnNitro, 0.35f, 6, colNitro);
    DrawRectangleRoundedLinesEx(m_btnNitro, 0.35f, 6, 2.5f, WHITE);
    DrawText("NITRO", (int)(m_btnNitro.x + m_btnNitro.width * 0.15f), (int)(m_btnNitro.y + m_btnNitro.height * 0.25f), 22, WHITE);
}

void UI::DrawCountdown(float countdownTimer) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    const char* cdText = "3";
    Color cdColor = RED;
    if (countdownTimer > 2.0f) { cdText = "3"; cdColor = RED; }
    else if (countdownTimer > 1.0f) { cdText = "2"; cdColor = ORANGE; }
    else if (countdownTimer > 0.0f) { cdText = "1"; cdColor = GOLD; }
    else { cdText = "GO!"; cdColor = GREEN; }

    int fontSize = 110;
    int textW = MeasureText(cdText, fontSize);
    DrawText(cdText, (screenW - textW) / 2, screenH / 2 - 120, fontSize, cdColor);
}

void UI::DrawResults(bool playerWon, float totalTime, float bestLap, bool& outRestart) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Dark semi-transparent backdrop
    DrawRectangle(0, 0, screenW, screenH, Color{ 5, 8, 15, 210 });

    Rectangle card = { (float)screenW * 0.5f - 240.0f, (float)screenH * 0.5f - 180.0f, 480.0f, 360.0f };
    DrawRectangleRounded(card, 0.15f, 6, Color{ 20, 26, 38, 255 });
    DrawRectangleRoundedLinesEx(card, 0.15f, 6, 3.0f, playerWon ? GOLD : Color{ 100, 120, 150, 255 });

    if (playerWon) {
        DrawText("VICTORY! 🏆", (int)card.x + 130, (int)card.y + 35, 38, GOLD);
        DrawText("You defeated the Bot!", (int)card.x + 140, (int)card.y + 85, 20, RAYWHITE);
    } else {
        DrawText("2ND PLACE", (int)card.x + 155, (int)card.y + 35, 38, Color{ 240, 120, 40, 255 });
        DrawText("Bot crossed the finish line first!", (int)card.x + 105, (int)card.y + 85, 20, RAYWHITE);
    }

    // Time statistics
    int mins = (int)totalTime / 60;
    float secs = totalTime - (mins * 60);
    char totalStr[64];
    snprintf(totalStr, sizeof(totalStr), "Total Race Time: %02d:%05.2f", mins, secs);
    DrawText(totalStr, (int)card.x + 80, (int)card.y + 145, 22, WHITE);

    if (bestLap < 900.0f) {
        int bMins = (int)bestLap / 60;
        float bSecs = bestLap - (bMins * 60);
        char bestStr[64];
        snprintf(bestStr, sizeof(bestStr), "Best Lap: %02d:%05.2f", bMins, bSecs);
        DrawText(bestStr, (int)card.x + 130, (int)card.y + 185, 22, Color{ 0, 240, 180, 255 });
    }

    // Play Again button
    Rectangle restartBtn = { card.x + 100.0f, card.y + 250.0f, 280.0f, 55.0f };
    Vector2 mouse = GetMousePosition();
    bool hover = CheckCollisionPointRec(mouse, restartBtn);
    if (GetTouchPointCount() > 0) {
        hover = CheckCollisionPointRec(GetTouchPosition(0), restartBtn);
    }

    DrawRectangleRounded(restartBtn, 0.3f, 6, hover ? Color{ 0, 220, 100, 255 } : Color{ 0, 170, 75, 255 });
    DrawRectangleRoundedLinesEx(restartBtn, 0.3f, 6, 2.0f, WHITE);
    DrawText("PLAY AGAIN", (int)restartBtn.x + 65, (int)restartBtn.y + 16, 24, WHITE);

    if (hover && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0 || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE))) {
        outRestart = true;
    }
}
