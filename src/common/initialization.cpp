#include "initialization.hpp"

#include <random>

std::vector<Body> create_bodies(const SimConfig& cfg) {
    // mt19937_64 gives the same sequence for the same seed on any machine.
    std::mt19937_64 rng(cfg.seed);
    std::uniform_real_distribution<double> position(cfg.radius, cfg.space - cfg.radius);
    std::uniform_real_distribution<double> mass(cfg.min_mass, cfg.max_mass);

    std::vector<Body> bodies(static_cast<size_t>(cfg.bodies));
    for (Body& b : bodies) {
        // Separate statements, so the order of random numbers is fixed.
        b.x = position(rng);
        b.y = position(rng);
        b.m = mass(rng);
    }
    return bodies;
}
