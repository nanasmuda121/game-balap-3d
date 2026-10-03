#include "ui.h"
#include <cstdio>
#include <algorithm>

UI::UI() {}
UI::~UI() {}

void UI::Init() {}

TouchInputState UI::ProcessInput(bool isMobile) {
    TouchInputState input = { 0.0f, 0.0f, false, false, false, false, false };
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // 1. Dynamic Responsive Button Dimensions
    float btnSize = screenH * 0.17f;
    if (btnSize < 70.0f) btnSize = 70.0f;
    if (btnSize > 115.0f) btnSize = 115.0f;

    // Steering Buttons (Bottom Left)
    m_btnLeft  = Rectangle{ 25.0f, screenH - btnSize - 25.0f, btnSize, btnSize };
    m_btnRight = Rectangle{ 35.0f + btnSize, screenH - btnSize - 25.0f, btnSize, btnSize };

    // Pedals & Nitro (Bottom Right)
    m_btnGas     = Rectangle{ screenW - btnSize - 25.0f, screenH - btnSize * 1.45f - 20.0f, btnSize, btnSize * 1.45f };
    m_btnBrake   = Rectangle{ screenW - btnSize * 2.1f - 35.0f, screenH - btnSize - 20.0f, btnSize * 0.95f, btnSize };
    m_btnReverse = Rectangle{ screenW - btnSize * 3.15f - 45.0f, screenH - btnSize - 20.0f, btnSize * 0.95f, btnSize };
    m_btnNitro   = Rectangle{ screenW - btnSize * 2.1f - 35.0f, screenH - btnSize * 2.2f - 30.0f, btnSize * 0.95f, btnSize * 0.7f };

    // Header Action Buttons
    m_btnMenu  = Rectangle{ 20.0f, 20.0f, 100.0f, 38.0f };
    m_btnReset = Rectangle{ screenW * 0.5f - 60.0f, 20.0f, 120.0f, 38.0f };

    // 2. Keyboard Controls (PC / Emulators)
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) input.throttle = 1.0f;
    if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) input.brake = true;
    if (IsKeyDown(KEY_B) || IsKeyDown(KEY_X)) input.reverse = true;
    if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) input.steer -= 1.0f;
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) input.steer += 1.0f;
    if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_SHIFT)) input.nitro = true;
    if (IsKeyPressed(KEY_R)) input.reset = true;
    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_ESCAPE)) input.menu = true;

    // 3. Multi-Touch Screen Inputs (Android)
    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 touch = GetTouchPosition(i);

        if (CheckCollisionPointRec(touch, m_btnLeft))    input.steer -= 1.0f;
        if (CheckCollisionPointRec(touch, m_btnRight))   input.steer += 1.0f;
        if (CheckCollisionPointRec(touch, m_btnGas))     input.throttle = 1.0f;
        if (CheckCollisionPointRec(touch, m_btnBrake))   input.brake = true;
        if (CheckCollisionPointRec(touch, m_btnReverse)) input.reverse = true;
        if (CheckCollisionPointRec(touch, m_btnNitro))   input.nitro = true;
        if (CheckCollisionPointRec(touch, m_btnReset))   input.reset = true;
        if (CheckCollisionPointRec(touch, m_btnMenu))    input.menu = true;
    }

    // Mouse fallback for PC clicks
    if (touchCount == 0 && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouse = GetMousePosition();
        if (CheckCollisionPointRec(mouse, m_btnLeft))    input.steer -= 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnRight))   input.steer += 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnGas))     input.throttle = 1.0f;
        if (CheckCollisionPointRec(mouse, m_btnBrake))   input.brake = true;
        if (CheckCollisionPointRec(mouse, m_btnReverse)) input.reverse = true;
        if (CheckCollisionPointRec(mouse, m_btnNitro))   input.nitro = true;
        if (CheckCollisionPointRec(mouse, m_btnReset))   input.reset = true;
        if (CheckCollisionPointRec(mouse, m_btnMenu))    input.menu = true;
    }

    input.steer = std::max(-1.0f, std::min(1.0f, input.steer));
    return input;
}

void UI::DrawMenu(GameMode& outMode, bool& outSelected) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Dark sleek gradient background
    DrawRectangle(0, 0, screenW, screenH, Color{ 12, 16, 26, 255 });

    // Header Title
    DrawText("RACEDRIVE 3D", screenW / 2 - MeasureText("RACEDRIVE 3D", 48) / 2, 60, 48, GOLD);
    DrawText("Pilih Mode Permainan:", screenW / 2 - MeasureText("Pilih Mode Permainan:", 22) / 2, 120, 22, RAYWHITE);

    float cardW = screenW * 0.38f;
    if (cardW < 260.0f) cardW = 260.0f;
    float cardH = screenH * 0.52f;
    float cardY = 170.0f;

    Rectangle cardRace = { screenW * 0.5f - cardW - 20.0f, cardY, cardW, cardH };
    Rectangle cardCity = { screenW * 0.5f + 20.0f, cardY, cardW, cardH };

    Vector2 mouse = GetMousePosition();
    bool hoverRace = CheckCollisionPointRec(mouse, cardRace);
    bool hoverCity = CheckCollisionPointRec(mouse, cardCity);

    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 t = GetTouchPosition(i);
        if (CheckCollisionPointRec(t, cardRace)) hoverRace = true;
        if (CheckCollisionPointRec(t, cardCity)) hoverCity = true;
    }

    // --- Card 1: BALAPAN (RACE VS BOT) ---
    DrawRectangleRounded(cardRace, 0.15f, 6, hoverRace ? Color{ 25, 45, 80, 255 } : Color{ 18, 25, 42, 255 });
    DrawRectangleRoundedLinesEx(cardRace, 0.15f, 6, 3.0f, hoverRace ? Color{ 0, 200, 255, 255 } : Color{ 60, 80, 120, 255 });

    DrawText("🏁 BALAPAN", (int)cardRace.x + 30, (int)cardRace.y + 35, 30, Color{ 0, 220, 255, 255 });
    DrawText("Sirkuit 3 Laps vs AI Bot", (int)cardRace.x + 30, (int)cardRace.y + 80, 18, WHITE);
    DrawText("- Balapan Grand Prix\n- Lawan AI Pintar\n- Catat Rekor Best Lap\n- Deteksi Tabrakan", (int)cardRace.x + 30, (int)cardRace.y + 120, 16, Color{ 180, 195, 220, 255 });

    Rectangle btnPlay1 = { cardRace.x + 30, cardRace.y + cardH - 65, cardW - 60, 45 };
    DrawRectangleRounded(btnPlay1, 0.25f, 6, Color{ 0, 150, 240, 255 });
    DrawText("MAIN BALAPAN", (int)btnPlay1.x + (int)btnPlay1.width/2 - MeasureText("MAIN BALAPAN", 18)/2, (int)btnPlay1.y + 13, 18, WHITE);

    // --- Card 2: OPEN WORLD (KOTA BEBAS) ---
    DrawRectangleRounded(cardCity, 0.15f, 6, hoverCity ? Color{ 35, 75, 45, 255 } : Color{ 20, 38, 28, 255 });
    DrawRectangleRoundedLinesEx(cardCity, 0.15f, 6, 3.0f, hoverCity ? Color{ 40, 220, 100, 255 } : Color{ 50, 120, 70, 255 });

    DrawText("🏙️ OPEN WORLD", (int)cardCity.x + 30, (int)cardCity.y + 35, 30, Color{ 60, 240, 120, 255 });
    DrawText("Jelajah Kota & Stunts", (int)cardCity.x + 30, (int)cardCity.y + 80, 18, WHITE);
    DrawText("- Kota 3D dengan Gedung\n- Stunt Ramps untuk Lompat\n- Bebas Drift & Test Speed\n- Tanpa Batas Waktu", (int)cardCity.x + 30, (int)cardCity.y + 120, 16, Color{ 180, 220, 195, 255 });

    Rectangle btnPlay2 = { cardCity.x + 30, cardCity.y + cardH - 65, cardW - 60, 45 };
    DrawRectangleRounded(btnPlay2, 0.25f, 6, Color{ 30, 180, 75, 255 });
    DrawText("MAIN OPEN WORLD", (int)btnPlay2.x + (int)btnPlay2.width/2 - MeasureText("MAIN OPEN WORLD", 18)/2, (int)btnPlay2.y + 13, 18, WHITE);

    // Handle Selection Click
    bool clicked = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (touchCount > 0);
    if (clicked) {
        if (hoverRace) {
            outMode = MODE_BALAPAN;
            outSelected = true;
        } else if (hoverCity) {
            outMode = MODE_OPENWORLD;
            outSelected = true;
        }
    }
}

void UI::DrawHUD(const Car& player, const Bot* bot, const Track* track, GameMode mode, GameState state, float countdownTimer) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // 1. Speedometer & Nitro (Bottom Center)
    DrawSpeedometer(player.GetSpeedKmh(), player.GetNitro(), screenW, screenH);

    // 2. Mode-Specific Information
    if (mode == MODE_BALAPAN && bot && track) {
        float playerDist = player.GetLap() * track->GetTotalLength() + track->GetProgressAlongTrack(player.GetPosition());
        float botDist = bot->GetCar().GetLap() * track->GetTotalLength() + track->GetProgressAlongTrack(bot->GetCar().GetPosition());
        int position = (playerDist >= botDist) ? 1 : 2;

        DrawRaceStatus(player.GetLap(), position, player.GetLapTime(), player.GetBestLapTime(), screenW, screenH);

        // Circuit Radar Mini-Map (Top Right)
        float mapSize = std::min(screenW * 0.22f, 170.0f);
        Rectangle mapBounds = { screenW - mapSize - 20.0f, 20.0f, mapSize, mapSize };
        const_cast<Track*>(track)->DrawMiniMap(mapBounds, player.GetPosition(), bot->GetCar().GetPosition(), player.GetYaw(), bot->GetCar().GetYaw());
    } else {
        // Open World Badge
        Rectangle modeBadge = { 130.0f, 20.0f, 180.0f, 40.0f };
        DrawRectangleRounded(modeBadge, 0.3f, 6, Color{ 20, 60, 35, 220 });
        DrawRectangleRoundedLinesEx(modeBadge, 0.3f, 6, 2.0f, Color{ 60, 220, 100, 255 });
        DrawText("🏙️ OPEN WORLD", (int)modeBadge.x + 18, (int)modeBadge.y + 11, 17, Color{ 100, 240, 140, 255 });

        // Stunt / Drift notification
        if (player.IsDrifting()) {
            DrawText("🔥 DRIFTING!", screenW / 2 - 60, screenH / 2 + 50, 24, ORANGE);
        }
    }

    // 3. On-Screen Touch Controls (Mobile Friendly)
    TouchInputState dummyState = ProcessInput(true);
    DrawTouchControls(screenW, screenH, dummyState);

    // 4. Header Control Buttons (Reset and Menu)
    DrawRectangleRounded(m_btnReset, 0.3f, 6, Color{ 40, 45, 60, 200 });
    DrawRectangleRoundedLinesEx(m_btnReset, 0.3f, 6, 2.0f, Color{ 100, 115, 145, 255 });
    DrawText("RESET [R]", (int)m_btnReset.x + 20, (int)m_btnReset.y + 11, 16, WHITE);

    DrawRectangleRounded(m_btnMenu, 0.3f, 6, Color{ 60, 30, 30, 200 });
    DrawRectangleRoundedLinesEx(m_btnMenu, 0.3f, 6, 2.0f, Color{ 180, 80, 80, 255 });
    DrawText("< MENU", (int)m_btnMenu.x + 16, (int)m_btnMenu.y + 11, 16, WHITE);
}

void UI::DrawSpeedometer(float speedKmh, float nitro, int screenW, int screenH) {
    int centerX = screenW / 2;
    int centerY = screenH - 52;

    Rectangle bg = { (float)centerX - 125.0f, (float)centerY - 45.0f, 250.0f, 85.0f };
    DrawRectangleRounded(bg, 0.3f, 6, Color{ 15, 18, 25, 220 });
    DrawRectangleRoundedLinesEx(bg, 0.3f, 6, 2.0f, Color{ 45, 55, 75, 255 });

    char speedText[32];
    snprintf(speedText, sizeof(speedText), "%3.0f", fabsf(speedKmh));
    DrawText(speedText, centerX - 55, centerY - 38, 42, WHITE);
    DrawText("KM/H", centerX + 25, centerY - 20, 17, Color{ 160, 180, 210, 255 });

    if (speedKmh < -0.5f) {
        DrawText("REV", centerX - 105, centerY - 35, 16, ORANGE);
    }

    // Nitro Bar
    DrawText("NITRO", centerX - 105, centerY + 18, 12, Color{ 0, 200, 255, 255 });
    Rectangle nitroBg = { (float)centerX - 55.0f, (float)centerY + 18.0f, 155.0f, 13.0f };
    DrawRectangleRec(nitroBg, Color{ 30, 35, 45, 255 });
    float fillW = (nitro / 100.0f) * 155.0f;
    DrawRectangleRec(Rectangle{ nitroBg.x, nitroBg.y, fillW, nitroBg.height }, Color{ 0, 220, 255, 255 });
    DrawRectangleLinesEx(nitroBg, 1.0f, Color{ 80, 100, 130, 255 });
}

void UI::DrawRaceStatus(int lap, int position, float lapTime, float bestLap, int screenW, int screenH) {
    Rectangle posBadge = { 130.0f, 20.0f, 85.0f, 85.0f };
    Color badgeColor = (position == 1) ? Color{ 0, 180, 255, 230 } : Color{ 230, 120, 20, 230 };
    DrawRectangleRounded(posBadge, 0.25f, 6, badgeColor);
    DrawRectangleRoundedLinesEx(posBadge, 0.25f, 6, 2.0f, WHITE);

    char posStr[8];
    snprintf(posStr, sizeof(posStr), "%dst", position);
    if (position == 2) snprintf(posStr, sizeof(posStr), "2nd");
    DrawText(posStr, (int)posBadge.x + 14, (int)posBadge.y + 24, 34, WHITE);
    DrawText("POS", (int)posBadge.x + 28, (int)posBadge.y + 64, 11, Color{ 220, 240, 255, 255 });

    Rectangle lapBox = { 225.0f, 20.0f, 150.0f, 85.0f };
    DrawRectangleRounded(lapBox, 0.2f, 6, Color{ 20, 25, 35, 210 });
    DrawRectangleRoundedLinesEx(lapBox, 0.2f, 6, 2.0f, Color{ 55, 70, 95, 255 });

    char lapStr[32];
    snprintf(lapStr, sizeof(lapStr), "LAP %d / %d", lap, TOTAL_LAPS);
    DrawText(lapStr, (int)lapBox.x + 15, (int)lapBox.y + 12, 20, (lap == TOTAL_LAPS) ? GOLD : WHITE);

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
    // Steer Left (<) Button
    Color colLeft = (input.steer < -0.1f) ? Color{ 0, 200, 255, 220 } : Color{ 40, 50, 70, 160 };
    DrawRectangleRounded(m_btnLeft, 0.4f, 6, colLeft);
    DrawRectangleRoundedLinesEx(m_btnLeft, 0.4f, 6, 2.5f, WHITE);
    DrawText("<", (int)(m_btnLeft.x + m_btnLeft.width * 0.38f), (int)(m_btnLeft.y + m_btnLeft.height * 0.22f), 48, WHITE);

    // Steer Right (>) Button
    Color colRight = (input.steer > 0.1f) ? Color{ 0, 200, 255, 220 } : Color{ 40, 50, 70, 160 };
    DrawRectangleRounded(m_btnRight, 0.4f, 6, colRight);
    DrawRectangleRoundedLinesEx(m_btnRight, 0.4f, 6, 2.5f, WHITE);
    DrawText(">", (int)(m_btnRight.x + m_btnRight.width * 0.38f), (int)(m_btnRight.y + m_btnRight.height * 0.22f), 48, WHITE);

    // GAS Pedal (Green)
    Color colGas = (input.throttle > 0.1f) ? Color{ 40, 220, 80, 240 } : Color{ 25, 120, 50, 170 };
    DrawRectangleRounded(m_btnGas, 0.3f, 6, colGas);
    DrawRectangleRoundedLinesEx(m_btnGas, 0.3f, 6, 2.5f, WHITE);
    DrawText("GAS", (int)(m_btnGas.x + m_btnGas.width * 0.2f), (int)(m_btnGas.y + m_btnGas.height * 0.38f), 28, WHITE);

    // REM / BRAKE Pedal (Red)
    Color colBrake = input.brake ? Color{ 240, 50, 50, 240 } : Color{ 140, 30, 30, 170 };
    DrawRectangleRounded(m_btnBrake, 0.3f, 6, colBrake);
    DrawRectangleRoundedLinesEx(m_btnBrake, 0.3f, 6, 2.5f, WHITE);
    DrawText("REM", (int)(m_btnBrake.x + m_btnBrake.width * 0.22f), (int)(m_btnBrake.y + m_btnBrake.height * 0.32f), 22, WHITE);

    // MUNDUR / REVERSE Pedal (Orange)
    Color colRev = input.reverse ? Color{ 255, 140, 0, 240 } : Color{ 150, 80, 10, 170 };
    DrawRectangleRounded(m_btnReverse, 0.3f, 6, colRev);
    DrawRectangleRoundedLinesEx(m_btnReverse, 0.3f, 6, 2.5f, WHITE);
    DrawText("MUNDUR", (int)(m_btnReverse.x + 8), (int)(m_btnReverse.y + m_btnReverse.height * 0.36f), 15, WHITE);

    // NITRO Button (Cyan)
    Color colNitro = input.nitro ? Color{ 0, 240, 255, 255 } : Color{ 0, 130, 190, 180 };
    DrawRectangleRounded(m_btnNitro, 0.35f, 6, colNitro);
    DrawRectangleRoundedLinesEx(m_btnNitro, 0.35f, 6, 2.5f, WHITE);
    DrawText("NITRO", (int)(m_btnNitro.x + m_btnNitro.width * 0.18f), (int)(m_btnNitro.y + m_btnNitro.height * 0.25f), 20, WHITE);
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

void UI::DrawResults(bool playerWon, float totalTime, float bestLap, bool& outRestart, bool& outMenu) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, Color{ 5, 8, 15, 210 });

    Rectangle card = { (float)screenW * 0.5f - 240.0f, (float)screenH * 0.5f - 190.0f, 480.0f, 380.0f };
    DrawRectangleRounded(card, 0.15f, 6, Color{ 20, 26, 38, 255 });
    DrawRectangleRoundedLinesEx(card, 0.15f, 6, 3.0f, playerWon ? GOLD : Color{ 100, 120, 150, 255 });

    if (playerWon) {
        DrawText("VICTORY! 🏆", (int)card.x + 130, (int)card.y + 30, 38, GOLD);
        DrawText("Kamu mengalahkan Bot!", (int)card.x + 135, (int)card.y + 75, 20, RAYWHITE);
    } else {
        DrawText("JUARA 2", (int)card.x + 175, (int)card.y + 30, 38, Color{ 240, 120, 40, 255 });
        DrawText("Bot mencapai garis finish lebih dulu!", (int)card.x + 95, (int)card.y + 75, 20, RAYWHITE);
    }

    int mins = (int)totalTime / 60;
    float secs = totalTime - (mins * 60);
    char totalStr[64];
    snprintf(totalStr, sizeof(totalStr), "Total Waktu: %02d:%05.2f", mins, secs);
    DrawText(totalStr, (int)card.x + 120, (int)card.y + 130, 22, WHITE);

    if (bestLap < 900.0f) {
        int bMins = (int)bestLap / 60;
        float bSecs = bestLap - (bMins * 60);
        char bestStr[64];
        snprintf(bestStr, sizeof(bestStr), "Best Lap: %02d:%05.2f", bMins, bSecs);
        DrawText(bestStr, (int)card.x + 140, (int)card.y + 168, 22, Color{ 0, 240, 180, 255 });
    }

    Rectangle restartBtn = { card.x + 50.0f, card.y + 240.0f, 180.0f, 50.0f };
    Rectangle menuBtn = { card.x + 250.0f, card.y + 240.0f, 180.0f, 50.0f };

    Vector2 mouse = GetMousePosition();
    bool hoverRestart = CheckCollisionPointRec(mouse, restartBtn);
    bool hoverMenu = CheckCollisionPointRec(mouse, menuBtn);

    int touchCount = GetTouchPointCount();
    for (int i = 0; i < touchCount; i++) {
        Vector2 t = GetTouchPosition(i);
        if (CheckCollisionPointRec(t, restartBtn)) hoverRestart = true;
        if (CheckCollisionPointRec(t, menuBtn)) hoverMenu = true;
    }

    DrawRectangleRounded(restartBtn, 0.3f, 6, hoverRestart ? Color{ 0, 220, 100, 255 } : Color{ 0, 170, 75, 255 });
    DrawRectangleRoundedLinesEx(restartBtn, 0.3f, 6, 2.0f, WHITE);
    DrawText("MAIN LAGI", (int)restartBtn.x + 35, (int)restartBtn.y + 15, 20, WHITE);

    DrawRectangleRounded(menuBtn, 0.3f, 6, hoverMenu ? Color{ 60, 90, 160, 255 } : Color{ 40, 60, 120, 255 });
    DrawRectangleRoundedLinesEx(menuBtn, 0.3f, 6, 2.0f, WHITE);
    DrawText("KE MENU", (int)menuBtn.x + 45, (int)menuBtn.y + 15, 20, WHITE);

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || touchCount > 0) {
        if (hoverRestart) outRestart = true;
        if (hoverMenu) outMenu = true;
    }
}
