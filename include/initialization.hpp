#pragma once
#include <vector>

#include "body.hpp"
#include "configuration.hpp"

// Creates the initial bodies. The same config (especially the seed) always
// gives exactly the same bodies, so sequential and MPI runs start identically.
std::vector<Body> create_bodies(const SimConfig& cfg);
