#include "physics.hpp"

#include <cmath>

void apply_wall_collision(Body& b, const SimConfig& cfg) {
    const double low = cfg.radius;
    const double high = cfg.space - cfg.radius;

    if (b.x < low)       { b.x = low;  b.vx =  std::fabs(b.vx); }
    else if (b.x > high) { b.x = high; b.vx = -std::fabs(b.vx); }

    if (b.y < low)       { b.y = low;  b.vy =  std::fabs(b.vy); }
    else if (b.y > high) { b.y = high; b.vy = -std::fabs(b.vy); }
}

Body compute_next_body(const Body* current, int n, int i, const SimConfig& cfg) {
    const Body& bi = current[i];
    const double contact = contact_distance(cfg);
    const double contact_sq = contact * contact;

    double ax = 0.0, ay = 0.0;           // gravity acceleration
    double dvx = 0.0, dvy = 0.0;         // velocity change from collisions
    double shift_x = 0.0, shift_y = 0.0; // position push from collisions
    int contacts = 0;                    // how many bodies touch body i

    // Always loop j = 0..n-1 in the same order. This fixes the order of the
    // floating point additions, so every program gets identical results.
    for (int j = 0; j < n; ++j) {
        if (j == i) continue;
        const Body& bj = current[j];

        const double dx = bi.x - bj.x;
        const double dy = bi.y - bj.y;
        const double dist_sq = dx * dx + dy * dy;

        if (dist_sq <= contact_sq) {
            // ---- collision: direction from j to i ----
            double nx, ny, dist;
            if (dist_sq > kMinDistance * kMinDistance) {
                dist = std::sqrt(dist_sq);
                nx = dx / dist;
                ny = dy / dist;
            } else {
                // Same position: pick a fixed direction. The lower index goes
                // +x and the higher index goes -x, so the two bodies separate.
                dist = 0.0;
                nx = (i < j) ? 1.0 : -1.0;
                ny = 0.0;
            }

            // Velocity along the collision direction. Negative = approaching.
            const double approach = (bi.vx - bj.vx) * nx + (bi.vy - bj.vy) * ny;
            const double total_mass = bi.m + bj.m;
            if (approach < 0.0 && total_mass > 0.0) {
                // Elastic collision impulse on body i only. Body j computes
                // its own half when it is the target.
                const double factor = 2.0 * bj.m / total_mass * approach;
                dvx -= factor * nx;
                dvy -= factor * ny;
            }

            ++contacts;

            // Each body moves half of the overlap (plus a small margin).
            const double push = 0.5 * (contact - dist) * (1.0 + kCollisionRatio);
            shift_x += push * nx;
            shift_y += push * ny;
        } else {
            // ---- gravity ----
            // Here dist_sq > contact_sq > 0, so there is no division by zero.
            const double dist = std::sqrt(dist_sq);
            const double scale = cfg.gravity * bj.m / (dist_sq * dist);
            ax -= scale * dx;
            ay -= scale * dy;
        }
    }

    // A body touching several others gets the AVERAGE of their responses.
    // Adding them up (all computed from the same snapshot) would make a body
    // bounce back faster than it arrived, and energy would grow every step.
    if (contacts > 1) {
        const double inv = 1.0 / contacts;
        dvx *= inv; dvy *= inv;
        shift_x *= inv; shift_y *= inv;
    }

    // Semi implicit Euler: update velocity first, then position with the new velocity.
    Body next = bi;
    next.ax = ax;
    next.ay = ay;
    next.vx = bi.vx + ax * cfg.dt + dvx;
    next.vy = bi.vy + ay * cfg.dt + dvy;
    next.x = bi.x + next.vx * cfg.dt + shift_x;
    next.y = bi.y + next.vy * cfg.dt + shift_y;
    apply_wall_collision(next, cfg);
    return next;
}

int find_invalid_body(const Body* bodies, int begin, int end) {
    for (int i = begin; i < end; ++i) {
        const Body& b = bodies[i];
        if (!std::isfinite(b.x) || !std::isfinite(b.y) ||
            !std::isfinite(b.vx) || !std::isfinite(b.vy) ||
            !std::isfinite(b.ax) || !std::isfinite(b.ay)) {
            return i;
        }
    }
    return -1;
}
