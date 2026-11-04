#include "simulation.hpp"

#include "physics.hpp"

void compute_step_range(const Body* current, Body* next, int n, int begin, int end,
                        const SimConfig& cfg) {
    for (int i = begin; i < end; ++i) {
        next[i] = compute_next_body(current, n, i, cfg);
    }
}

void step_sequential(const std::vector<Body>& current, std::vector<Body>& next,
                     const SimConfig& cfg) {
    const int n = static_cast<int>(current.size());
    next.resize(current.size());
    compute_step_range(current.data(), next.data(), n, 0, n, cfg);
}
