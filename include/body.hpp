#pragma once
#include <type_traits>

// One simulated body.
// x, y, vx, vy are the state that changes every step (Stage 5 sends only these
// four over MPI). ax, ay are the gravity acceleration of the last step, kept
// for diagnostics. m never changes.
struct Body {
    double x = 0.0;
    double y = 0.0;
    double vx = 0.0;
    double vy = 0.0;
    double ax = 0.0;
    double ay = 0.0;
    double m = 1.0;
};

// Needed later so a Body array can be copied and sent as raw memory.
static_assert(std::is_standard_layout<Body>::value, "Body must be standard layout");
static_assert(std::is_trivially_copyable<Body>::value, "Body must be trivially copyable");
