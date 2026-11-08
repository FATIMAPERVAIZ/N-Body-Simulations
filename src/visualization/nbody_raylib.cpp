// Real-time N-Body visualization using Raylib
// Compile instructions are at the bottom.

#include "raylib.h"
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <string>
#include <iostream>

// Simple body
struct Particle {
    Vector2 pos;
    Vector2 vel;
    float mass;
    Color color;
};

// Configuration
const int SCREEN_W = 1000;
const int SCREEN_H = 700;
const float WORLD_SIZE = 800.0f;   // same as your simulation space
const float DT = 0.02f;
const float G = 80.0f;            // slightly reduced for visual stability
const float SOFTENING = 4.0f;
const int NUM_PARTICLES = 1500;   // start with 1000-3000 for smooth real-time

// Convert world coordinates to screen
Vector2 world_to_screen(Vector2 p) {
    float scale = std::min(SCREEN_W, SCREEN_H) / WORLD_SIZE * 0.9f;
    float offset_x = SCREEN_W * 0.5f;
    float offset_y = SCREEN_H * 0.5f;
    return {
        offset_x + (p.x - WORLD_SIZE * 0.5f) * scale,
        offset_y - (p.y - WORLD_SIZE * 0.5f) * scale   // flip Y for screen
    };
}

// Create initial particles
std::vector<Particle> create_particles(int n) {
    std::vector<Particle> particles(n);
    std::srand(42);

    for (int i = 0; i < n; ++i) {
        particles[i].pos = {
            WORLD_SIZE * 0.1f + (float)rand() / RAND_MAX * WORLD_SIZE * 0.8f,
            WORLD_SIZE * 0.1f + (float)rand() / RAND_MAX * WORLD_SIZE * 0.8f
        };
        particles[i].vel = {
            ((float)rand() / RAND_MAX - 0.5f) * 30.0f,
            ((float)rand() / RAND_MAX - 0.5f) * 30.0f
        };
        particles[i].mass = 1.0f + (float)rand() / RAND_MAX * 8.0f;

        // Color by mass
        float t = (particles[i].mass - 1.0f) / 8.0f;
        particles[i].color = Color{
            (unsigned char)(50 + t * 200),
            (unsigned char)(100 + t * 100),
            (unsigned char)(255 - t * 100),
            220
        };
    }
    return particles;
}

// Direct O(N²) force (good enough for real-time with N <= ~3000)
void update_particles(std::vector<Particle>& p, float dt) {
    const int n = (int)p.size();
    std::vector<Vector2> acc(n, {0.0f, 0.0f});

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            float dx = p[j].pos.x - p[i].pos.x;
            float dy = p[j].pos.y - p[i].pos.y;
            float dist2 = dx*dx + dy*dy + SOFTENING*SOFTENING;
            float dist = sqrtf(dist2);
            float force = G * p[i].mass * p[j].mass / (dist2 * dist);

            float fx = force * dx;
            float fy = force * dy;

            acc[i].x += fx / p[i].mass;
            acc[i].y += fy / p[i].mass;
            acc[j].x -= fx / p[j].mass;
            acc[j].y -= fy / p[j].mass;
        }
    }

    // Integrate + simple wall bounce
    for (int i = 0; i < n; ++i) {
        p[i].vel.x += acc[i].x * dt;
        p[i].vel.y += acc[i].y * dt;
        p[i].pos.x += p[i].vel.x * dt;
        p[i].pos.y += p[i].vel.y * dt;

        // Walls
        if (p[i].pos.x < 0)               { p[i].pos.x = 0;               p[i].vel.x *= -0.8f; }
        if (p[i].pos.x > WORLD_SIZE)      { p[i].pos.x = WORLD_SIZE;      p[i].vel.x *= -0.8f; }
        if (p[i].pos.y < 0)               { p[i].pos.y = 0;               p[i].vel.y *= -0.8f; }
        if (p[i].pos.y > WORLD_SIZE)      { p[i].pos.y = WORLD_SIZE;      p[i].vel.y *= -0.8f; }
    }
}

int main() {
    InitWindow(SCREEN_W, SCREEN_H, "Real-time N-Body Simulation (Raylib)");
    SetTargetFPS(60);

    std::vector<Particle> particles = create_particles(NUM_PARTICLES);

    bool paused = false;
    int frame = 0;

    while (!WindowShouldClose()) {
        // Input
        if (IsKeyPressed(KEY_SPACE)) paused = !paused;
        if (IsKeyPressed(KEY_R)) {
            particles = create_particles(NUM_PARTICLES);
            frame = 0;
        }

        // Update physics
        if (!paused) {
            update_particles(particles, DT);
            frame++;
        }

        // Draw
        BeginDrawing();
        ClearBackground(Color{10, 10, 25, 255});

        // Draw particles
        for (const auto& p : particles) {
            Vector2 screen = world_to_screen(p.pos);
            float radius = 1.5f + p.mass * 0.4f;
            DrawCircleV(screen, radius, p.color);
        }

        // UI
        DrawText("Real-time N-Body (Raylib)", 20, 15, 24, RAYWHITE);
        DrawText(TextFormat("Particles: %d", NUM_PARTICLES), 20, 50, 20, LIGHTGRAY);
        DrawText(TextFormat("Frame: %d", frame), 20, 75, 20, LIGHTGRAY);
        DrawText("SPACE = Pause/Resume    R = Reset", 20, SCREEN_H - 40, 18, GRAY);

        if (paused) {
            DrawText("PAUSED", SCREEN_W/2 - 50, 30, 28, YELLOW);
        }

        DrawFPS(SCREEN_W - 100, 15);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
