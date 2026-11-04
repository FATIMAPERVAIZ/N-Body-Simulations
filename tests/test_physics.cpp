// Stage 4: checks for initialization, physics and the step workflow.
// No test framework needed: each CHECK prints ok or FAIL.
#include <cmath>
#include <iostream>
#include <vector>

#include "body.hpp"
#include "configuration.hpp"
#include "initialization.hpp"
#include "physics.hpp"
#include "simulation.hpp"

static int g_failures = 0;

#define CHECK(cond, msg)                                                  \
    do {                                                                  \
        if (cond) {                                                       \
            std::cout << "ok:   " << msg << "\n";                         \
        } else {                                                          \
            std::cout << "FAIL: " << msg << "   [" #cond "]\n";           \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

static SimConfig base_config() {
    SimConfig c;
    c.space = 800.0;
    c.radius = 5.0;
    c.dt = 0.02;
    c.gravity = 100.0;
    return c;
}

static Body make_body(double x, double y, double vx, double vy, double m) {
    Body b;
    b.x = x; b.y = y; b.vx = vx; b.vy = vy; b.m = m;
    return b;
}

static bool same_state(const std::vector<Body>& a, const std::vector<Body>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i].x != b[i].x || a[i].y != b[i].y || a[i].vx != b[i].vx ||
            a[i].vy != b[i].vy || a[i].m != b[i].m) {
            return false;
        }
    }
    return true;
}

static std::vector<Body> run(const SimConfig& cfg) {
    std::vector<Body> cur = create_bodies(cfg);
    std::vector<Body> next(cur.size());
    for (int s = 0; s < cfg.steps; ++s) {
        step_sequential(cur, next, cfg);
        cur.swap(next);
    }
    return cur;
}

static void test_initialization() {
    SimConfig cfg = base_config();
    cfg.bodies = 50;
    cfg.seed = 7;

    const std::vector<Body> a = create_bodies(cfg);
    const std::vector<Body> b = create_bodies(cfg);
    CHECK(a.size() == 50, "init creates the requested number of bodies");
    CHECK(same_state(a, b), "same seed gives identical bodies");

    cfg.seed = 8;
    CHECK(!same_state(a, create_bodies(cfg)), "different seed gives different bodies");

    bool in_range = true;
    for (const Body& body : a) {
        if (body.x < cfg.radius || body.x > cfg.space - cfg.radius ||
            body.y < cfg.radius || body.y > cfg.space - cfg.radius ||
            body.m < cfg.min_mass || body.m > cfg.max_mass) {
            in_range = false;
        }
    }
    CHECK(in_range, "initial positions and masses are inside their ranges");
}

static void test_zero_distance() {
    SimConfig cfg = base_config();
    std::vector<Body> cur = {make_body(400, 400, 0, 0, 1.0), make_body(400, 400, 0, 0, 2.0)};
    std::vector<Body> next;
    step_sequential(cur, next, cfg);

    CHECK(find_invalid_body(next.data(), 0, 2) == -1, "zero distance: no NaN or Inf");
    const double dx = next[0].x - next[1].x;
    const double dy = next[0].y - next[1].y;
    CHECK(std::sqrt(dx * dx + dy * dy) >= contact_distance(cfg),
          "zero distance: bodies are separated to at least contact distance");
}

static void test_head_on_collision() {
    SimConfig cfg = base_config();
    // Overlapping (distance 8 < 10) and moving towards each other.
    std::vector<Body> cur = {make_body(400, 400, 10, 0, 1.0), make_body(408, 400, -10, 0, 3.0)};
    std::vector<Body> next;
    step_sequential(cur, next, cfg);

    const double p_before = 1.0 * 10 + 3.0 * -10;
    const double p_after = next[0].m * next[0].vx + next[1].m * next[1].vx;
    const double e_before = 0.5 * 1.0 * 100 + 0.5 * 3.0 * 100;
    const double e_after = 0.5 * next[0].m * next[0].vx * next[0].vx +
                           0.5 * next[1].m * next[1].vx * next[1].vx;

    CHECK(std::fabs(next[0].vx - (-20.0)) < 1e-9, "collision: body 0 velocity matches elastic formula");
    CHECK(std::fabs(next[1].vx - 0.0) < 1e-9, "collision: body 1 velocity matches elastic formula");
    CHECK(std::fabs(p_after - p_before) < 1e-9, "collision: momentum is conserved");
    CHECK(std::fabs(e_after - e_before) < 1e-9, "collision: kinetic energy is conserved");
    CHECK(next[1].x - next[0].x > contact_distance(cfg), "collision: bodies are pushed apart");
}

static void test_separating_bodies_not_pulled_back() {
    SimConfig cfg = base_config();
    // Overlapping but already moving apart: velocities must not change.
    std::vector<Body> cur = {make_body(400, 400, -10, 0, 1.0), make_body(408, 400, 10, 0, 1.0)};
    std::vector<Body> next;
    step_sequential(cur, next, cfg);
    CHECK(next[0].vx == -10.0 && next[1].vx == 10.0,
          "separating overlap: velocities are left unchanged");
}

static void test_walls() {
    SimConfig cfg = base_config();
    std::vector<Body> cur = {make_body(6, 400, -100, 0, 1.0)};
    std::vector<Body> next;
    step_sequential(cur, next, cfg);
    CHECK(next[0].x >= cfg.radius && next[0].vx > 0, "left wall: body stays inside and bounces");

    cur = {make_body(400, 795, 0, 100, 1.0)};
    step_sequential(cur, next, cfg);
    CHECK(next[0].y <= cfg.space - cfg.radius && next[0].vy < 0,
          "top wall: body stays inside and bounces");
}

static void test_gravity_direction() {
    SimConfig cfg = base_config();
    std::vector<Body> cur = {make_body(100, 400, 0, 0, 10.0), make_body(300, 400, 0, 0, 10.0)};
    std::vector<Body> next;
    step_sequential(cur, next, cfg);

    const double expected = cfg.gravity * 10.0 / (200.0 * 200.0);  // G * m / d^2
    CHECK(std::fabs(next[0].ax - expected) < 1e-12, "gravity: body 0 is pulled towards body 1");
    CHECK(std::fabs(next[1].ax + expected) < 1e-12, "gravity: body 1 is pulled towards body 0");
}

static void test_determinism_and_slices() {
    SimConfig cfg = base_config();
    cfg.bodies = 60;
    cfg.steps = 30;
    cfg.space = 100.0;  // small world so collisions really happen

    const std::vector<Body> r1 = run(cfg);
    const std::vector<Body> r2 = run(cfg);
    CHECK(same_state(r1, r2), "full run twice gives bit for bit identical results");
    CHECK(find_invalid_body(r1.data(), 0, static_cast<int>(r1.size())) == -1,
          "full run has no NaN or Inf");

    // Uneven slices (23, 18, 19 bodies) must match one full step exactly.
    // This is what the MPI version relies on.
    const std::vector<Body> cur = create_bodies(cfg);
    std::vector<Body> full(cur.size()), sliced(cur.size());
    step_sequential(cur, full, cfg);
    const int n = static_cast<int>(cur.size());
    compute_step_range(cur.data(), sliced.data(), n, 0, 23, cfg);
    compute_step_range(cur.data(), sliced.data(), n, 23, 41, cfg);
    compute_step_range(cur.data(), sliced.data(), n, 41, 60, cfg);
    CHECK(same_state(full, sliced), "uneven slices give the same result as one full step");
}

static void test_dense_stability() {
    // 500 bodies in a small world: many bodies touch several others at once.
    // Before collision responses were averaged, energy grew until values became Inf.
    SimConfig cfg = base_config();
    cfg.bodies = 500;
    cfg.steps = 600;
    cfg.space = 400.0;

    const std::vector<Body> r = run(cfg);
    const int n = static_cast<int>(r.size());
    double max_speed = 0.0;
    for (const Body& b : r) {
        max_speed = std::fmax(max_speed, std::sqrt(b.vx * b.vx + b.vy * b.vy));
    }
    CHECK(find_invalid_body(r.data(), 0, n) == -1, "dense cluster: no NaN or Inf after 600 steps");
    CHECK(max_speed < 5000.0, "dense cluster: speeds stay bounded (energy does not blow up)");
}

int main() {
    test_initialization();
    test_zero_distance();
    test_head_on_collision();
    test_separating_bodies_not_pulled_back();
    test_walls();
    test_gravity_direction();
    test_determinism_and_slices();
    test_dense_stability();

    if (g_failures == 0) {
        std::cout << "\nAll checks passed.\n";
        return 0;
    }
    std::cout << "\n" << g_failures << " check(s) FAILED.\n";
    return 1;
}
