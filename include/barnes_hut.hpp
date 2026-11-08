#pragma once
#include <vector>
#include "body.hpp"
#include "configuration.hpp"

// Compute accelerations using Barnes-Hut quadtree.
// theta = opening angle (0.3–0.7 typical). Smaller = more accurate, slower.
void barnes_hut_accelerations(
    const std::vector<Body>& bodies,
    std::vector<double>& ax,
    std::vector<double>& ay,
    const SimConfig& cfg,
    double theta = 0.5);
