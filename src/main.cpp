#include "common.h"
#include "track.h"
#include "car.h"
#include "bot.h"
#include "camera_follow.h"
#include "ui.h"

int main(int argc, char *argv[]) {
    // 1. Initialize Display
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 720, "RaceDrive - Player vs Bot");
    SetTargetFPS(TARGET_FPS);

    // 2. Initialize Game Components
    Track track;
    Car player;
    Bot bot;
    CameraFollow camera;
    UI ui;

    // Start Grid setup (on Main Straightaway)
    const auto& startPt = track.GetPoints()[40];
    float startYaw = atan2f(-startPt.forward.x, -startPt.forward.z);

    Vector3 playerStartPos = Vector3Add(startPt.pos, Vector3Add(Vector3Scale(startPt.right, -3.2f), Vector3Scale(startPt.forward, -6.0f)));
    Vector3 botStartPos    = Vector3Add(startPt.pos, Vector3Add(Vector3Scale(startPt.right,  3.2f), Vector3Scale(startPt.forward, -2.0f)));

    // Initialize Player with converted model from mobil.3ma
    player.Init(playerStartPos, startYaw, "assets/player_car.obj", BLUE, true);

    // Initialize Bot with opponent livery
    bot.Init(botStartPos, startYaw, "assets/bot_car.obj", RED);

    camera.Reset(player);
    ui.Init();

    // 3. Race State
    GameState gameState = STATE_COUNTDOWN;
    float countdownTimer = 3.5f;
    float totalRaceTime = 0.0f;
    bool playerWon = false;

    auto ResetRace = [&]() {
        player.Reset(playerStartPos, startYaw);
        bot.Init(botStartPos, startYaw, "assets/bot_car.obj", RED);
        camera.Reset(player);
        gameState = STATE_COUNTDOWN;
        countdownTimer = 3.5f;
        totalRaceTime = 0.0f;
        playerWon = false;
    };

    // 4. Main Game Loop
    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        if (dt > 0.05f) dt = 0.05f; // Prevent huge deltas

        // Process Inputs (Keyboard + Touchscreen)
        TouchInputState input = ui.ProcessInput(false);

        if (input.reset) {
            // Reset player onto track centerline
            int nearestIdx = track.GetClosestIndex(player.GetPosition());
            const auto& pt = track.GetPoints()[nearestIdx];
            float yaw = atan2f(-pt.forward.x, -pt.forward.z);
            player.Reset(Vector3Add(pt.pos, Vector3{ 0.0f, 0.2f, 0.0f }), yaw);
            camera.Reset(player);
        }

        // --- UPDATE LOGIC ---
        if (gameState == STATE_COUNTDOWN) {
            countdownTimer -= dt;
            if (countdownTimer <= 0.0f) {
                gameState = STATE_RACING;
            }
            // Allow revving engine in place
            player.Update(dt, 0.0f, input.steer, false, false);
            camera.Update(dt, player);
        } 
        else if (gameState == STATE_RACING) {
            totalRaceTime += dt;

            // Update Player
            player.Update(dt, input.throttle, input.steer, input.brake, input.nitro);

            // Update Bot AI
            bot.Update(dt, track, player.GetPosition());

            // Track progress & Lap completion
            float playerProgress = track.GetProgressAlongTrack(player.GetPosition());
            player.UpdateLapProgress(playerProgress, track.GetTotalLength());

            // Check Barrier Collisions
            Vector3 pushPlayer;
            if (track.CheckBarrierCollision(player.GetPosition(), player.GetRadius(), pushPlayer)) {
                player.ApplyCollisionImpulse(pushPlayer);
                camera.AddShake(0.25f);
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
            }

            // Check Finish Condition (3 Laps)
            if (player.HasFinished() || bot.GetCar().HasFinished()) {
                gameState = STATE_FINISHED;
                playerWon = player.HasFinished() && (!bot.GetCar().HasFinished() || player.GetLapTime() <= bot.GetCar().GetLapTime());
            }

            camera.Update(dt, player);
        }
        else if (gameState == STATE_FINISHED) {
            player.Update(dt, 0.0f, 0.0f, true, false);
            bot.GetCar().Update(dt, 0.0f, 0.0f, true, false);
            camera.Update(dt, player);

            bool restart = false;
            // UI handles restart button in DrawResults
        }

        // --- RENDER PASS ---
        BeginDrawing();
        ClearBackground(Color{ 135, 206, 235, 255 }); // Sky Blue

        // 1. 3D World Rendering
        BeginMode3D(camera.GetCamera());
            // Render Circuit & Scenery
            track.Draw3D();

            // Render Bot Car (3D model from mobil.3ma)
            bot.Draw3D();

            // Render Player Car (3D model from mobil.3ma)
            player.Draw3D();
        EndMode3D();

        // 2. 2D HUD & Overlay Rendering
        ui.DrawHUD(player, bot, track, gameState, countdownTimer);

        if (gameState == STATE_COUNTDOWN) {
            ui.DrawCountdown(countdownTimer);
        } else if (gameState == STATE_FINISHED) {
            bool restart = false;
            ui.DrawResults(playerWon, totalRaceTime, player.GetBestLapTime(), restart);
            if (restart) {
                ResetRace();
            }
        }

        DrawFPS(GetScreenWidth() - 90, GetScreenHeight() - 25);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
