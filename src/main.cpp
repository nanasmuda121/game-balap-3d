#include "common.h"
#include "track.h"
#include "city.h"
#include "car.h"
#include "bot.h"
#include "camera_follow.h"
#include "ui.h"
#include "audio.h"

int main(int argc, char *argv[]) {
    // 1. Initialize Display
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "RaceDrive - 3D Racing & Open World");
    SetTargetFPS(TARGET_FPS);

    // 2. Initialize Audio System
    AudioSystem audio;
    audio.Init();

    // 3. Initialize Game Environments & Actors
    Track track;
    City city;
    city.Init();

    Car player;
    Bot bot;
    CameraFollow camera;
    UI ui;
    ui.Init();

    // Start Grid Setup for Balapan (Circuit)
    const auto& startPt = track.GetPoints()[40];
    float trackStartYaw = atan2f(-startPt.forward.x, -startPt.forward.z);
    Vector3 playerTrackStart = Vector3Add(startPt.pos, Vector3Add(Vector3Scale(startPt.right, -3.2f), Vector3Scale(startPt.forward, -6.0f)));
    Vector3 botTrackStart    = Vector3Add(startPt.pos, Vector3Add(Vector3Scale(startPt.right,  3.2f), Vector3Scale(startPt.forward, -2.0f)));

    // Start Setup for Open World (City Boulevard)
    Vector3 playerCityStart = Vector3{ 0.0f, 0.0f, 0.0f };
    float cityStartYaw = 0.0f;

    // Load 3D Models (mobil.obj for player, mobil2.obj for bot)
    player.Init(playerTrackStart, trackStartYaw, "player_car.obj", BLUE, true);
    bot.Init(botTrackStart, trackStartYaw, "bot_car.obj", RED);

    camera.Reset(player);

    // 4. Game Modes and States
    GameMode currentMode = MODE_BALAPAN;
    GameState gameState = STATE_MENU;

    float countdownTimer = 3.5f;
    int lastBeepSecond = 4;
    float totalRaceTime = 0.0f;
    bool playerWon = false;

    auto StartMode = [&](GameMode mode) {
        currentMode = mode;
        if (mode == MODE_BALAPAN) {
            player.Reset(playerTrackStart, trackStartYaw);
            bot.Init(botTrackStart, trackStartYaw, "bot_car.obj", RED);
            camera.Reset(player);
            gameState = STATE_COUNTDOWN;
            countdownTimer = 3.5f;
            lastBeepSecond = 4;
            totalRaceTime = 0.0f;
            playerWon = false;
        } else {
            // Open World
            player.Reset(playerCityStart, cityStartYaw);
            camera.Reset(player);
            gameState = STATE_PLAYING;
        }
    };

    // 5. Main Game Loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f;

        // Process User Inputs (Touchscreen & Keyboard)
        TouchInputState input = ui.ProcessInput(false);

        if (input.menu && gameState != STATE_MENU) {
            gameState = STATE_MENU;
        }

        // --- STATE: MENU ---
        if (gameState == STATE_MENU) {
            GameMode chosenMode = MODE_BALAPAN;
            bool selected = false;

            BeginDrawing();
            ClearBackground(Color{ 10, 15, 25, 255 });
            ui.DrawMenu(chosenMode, selected);
            EndDrawing();

            if (selected) {
                StartMode(chosenMode);
            }
            continue;
        }

        // --- STATE: BALAPAN & OPENWORLD GAMEPLAY ---
        if (input.reset) {
            if (currentMode == MODE_BALAPAN) {
                int nearestIdx = track.GetClosestIndex(player.GetPosition());
                const auto& pt = track.GetPoints()[nearestIdx];
                float yaw = atan2f(-pt.forward.x, -pt.forward.z);
                player.Reset(Vector3Add(pt.pos, Vector3{ 0.0f, 0.2f, 0.0f }), yaw);
            } else {
                player.Reset(Vector3{ 0.0f, 0.2f, 0.0f }, 0.0f);
            }
            camera.Reset(player);
        }

        // 1. Audio Updates
        float speedRatio = player.GetSpeed() / 38.0f;
        audio.Update(speedRatio, player.IsDrifting(), player.IsNitroActive());

        if (input.nitro && player.GetNitro() > 5.0f && input.throttle > 0.1f) {
            audio.PlayNitro();
        }

        // 2. State Countdown (Circuit Mode Only)
        if (gameState == STATE_COUNTDOWN) {
            countdownTimer -= dt;
            int curSecond = (int)ceilf(countdownTimer);
            if (curSecond > 0 && curSecond < lastBeepSecond) {
                audio.PlayCountdownBeep(false); // Beep for 3, 2, 1
                lastBeepSecond = curSecond;
            }

            if (countdownTimer <= 0.0f) {
                audio.PlayCountdownBeep(true); // High chord for GO!
                gameState = STATE_PLAYING;
            }
            // Allow stationary steering & engine revving
            player.Update(dt, 0.0f, input.steer, false, false, false);
            camera.Update(dt, player);
        }
        // 3. State Playing
        else if (gameState == STATE_PLAYING) {
            if (currentMode == MODE_BALAPAN) {
                totalRaceTime += dt;

                // Update Player
                player.Update(dt, input.throttle, input.steer, input.brake, input.reverse, input.nitro);

                // Update Bot AI
                bot.Update(dt, track, player.GetPosition());

                // Track Progress & Lap Count
                float playerProgress = track.GetProgressAlongTrack(player.GetPosition());
                player.UpdateLapProgress(playerProgress, track.GetTotalLength());

                // Barrier Collisions
                Vector3 pushPlayer;
                if (track.CheckBarrierCollision(player.GetPosition(), player.GetRadius(), pushPlayer)) {
                    player.ApplyCollisionImpulse(pushPlayer);
                    camera.AddShake(0.25f);
                    audio.PlayCollision();
                }

                Vector3 pushBot;
                if (track.CheckBarrierCollision(bot.GetCar().GetPosition(), bot.GetCar().GetRadius(), pushBot)) {
                    bot.GetCar().ApplyCollisionImpulse(pushBot);
                }

                // Car-vs-Car Elastic Collision
                Vector3 delta = Vector3Subtract(player.GetPosition(), bot.GetCar().GetPosition());
                delta.y = 0.0f;
                float dist = Vector3Length(delta);
                float minDist = player.GetRadius() + bot.GetCar().GetRadius();

                if (dist < minDist && dist > 0.001f) {
                    Vector3 normal = Vector3Scale(delta, 1.0f / dist);
                    float overlap = minDist - dist;
                    Vector3 impulse = Vector3Scale(normal, overlap * 0.6f);

                    player.ApplyCollisionImpulse(impulse);
                    bot.GetCar().ApplyCollisionImpulse(Vector3Scale(impulse, -1.0f));
                    camera.AddShake(0.35f);
                    audio.PlayCollision();
                }

                // Check Finish Condition
                if (player.HasFinished() || bot.GetCar().HasFinished()) {
                    gameState = STATE_FINISHED;
                    playerWon = player.HasFinished() && (!bot.GetCar().HasFinished() || player.GetLapTime() <= bot.GetCar().GetLapTime());
                }
            } 
            else {
                // Open World Mode: Support Stunt Ramps and City Collisions
                float groundHeight = city.GetSurfaceHeight(player.GetPosition());
                player.Update(dt, input.throttle, input.steer, input.brake, input.reverse, input.nitro, groundHeight);

                // City Building / Wall Collisions
                Vector3 pushOut;
                if (city.CheckCollision(player.GetPosition(), player.GetRadius(), pushOut)) {
                    player.ApplyCollisionImpulse(pushOut);
                    camera.AddShake(0.3f);
                    audio.PlayCollision();
                }
            }

            camera.Update(dt, player);
        }
        // 4. State Finished (Race Results)
        else if (gameState == STATE_FINISHED) {
            player.Update(dt, 0.0f, 0.0f, true, false, false);
            bot.GetCar().Update(dt, 0.0f, 0.0f, true, false, false);
            camera.Update(dt, player);
        }

        // --- RENDER PASS ---
        BeginDrawing();
        ClearBackground(Color{ 135, 206, 235, 255 }); // Sky Blue

        // 3D Scene Rendering
        BeginMode3D(camera.GetCamera());
            if (currentMode == MODE_BALAPAN) {
                track.Draw3D();
                bot.Draw3D();
            } else {
                city.Draw3D();
            }
            player.Draw3D();
        EndMode3D();

        // 2D HUD Rendering
        ui.DrawHUD(player, (currentMode == MODE_BALAPAN) ? &bot : nullptr, (currentMode == MODE_BALAPAN) ? &track : nullptr, currentMode, gameState, countdownTimer);

        if (gameState == STATE_COUNTDOWN) {
            ui.DrawCountdown(countdownTimer);
        } else if (gameState == STATE_FINISHED) {
            bool restart = false;
            bool toMenu = false;
            ui.DrawResults(playerWon, totalRaceTime, player.GetBestLapTime(), restart, toMenu);
            if (restart) {
                StartMode(MODE_BALAPAN);
            } else if (toMenu) {
                gameState = STATE_MENU;
            }
        }

        DrawFPS(GetScreenWidth() - 90, GetScreenHeight() - 25);

        EndDrawing();
    }

    // Cleanup
    audio.Close();
    CloseWindow();
    return 0;
}
