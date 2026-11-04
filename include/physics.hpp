#pragma once
#include "body.hpp"
#include "configuration.hpp"

// After a collision, bodies are pushed slightly more than the overlap so they
// do not stay in contact and trigger the same collision again.
constexpr double kCollisionRatio = 0.01;

// Distances below this are treated as "same position".
constexpr double kMinDistance = 1e-12;

inline double contact_distance(const SimConfig& cfg) { return 2.0 * cfg.radius; }

// Keeps a body inside the world and makes its velocity point inward.
void apply_wall_collision(Body& body, const SimConfig& cfg);

// Computes the state of body `i` at the next timestep.
// It only READS the snapshot `current` (n bodies) and returns a new Body, so
// the result never depends on the order in which bodies are processed.
Body compute_next_body(const Body* current, int n, int i, const SimConfig& cfg);

// Returns the index of the first body in [begin, end) with a NaN or infinite
// value, or -1 if all are valid.
int find_invalid_body(const Body* bodies, int begin, int end);
