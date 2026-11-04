#pragma once
#include <vector>

#include "body.hpp"
#include "configuration.hpp"

// Computes next[begin..end) from the snapshot `current` (all n bodies).
// The MPI program calls this with its own slice [begin, end).
void compute_step_range(const Body* current, Body* next, int n, int begin, int end,
                        const SimConfig& cfg);

// One full timestep for all bodies (sequential version).
void step_sequential(const std::vector<Body>& current, std::vector<Body>& next,
                     const SimConfig& cfg);
