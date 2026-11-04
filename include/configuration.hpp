#pragma once
#include <cstdint>
#include <string>

// All simulation parameters. Sequential and MPI programs use this same struct,
// so both always run the same physical model.
struct SimConfig {
    int bodies = 200;
    int steps = 100;
    std::uint64_t seed = 42;

    double dt = 0.02;         // timestep length
    double gravity = 100.0;   // gravitational constant G
    double space = 800.0;     // square world, coordinates in [0, space]
    double radius = 5.0;      // body radius (contact distance is 2 * radius)
    double min_mass = 1.0;
    double max_mass = 50.0;

    int snapshot_every = 0;   // 0 means no snapshots
    std::string output_file = "results/raw/final_state.csv";
    std::string snapshot_file = "results/raw/snapshots.csv";

    bool show_help = false;
};

// Reads --key value pairs. Returns false and fills `error` on bad input.
bool parse_config(int argc, char** argv, SimConfig& cfg, std::string& error);

// Checks that the values make physical and numerical sense.
bool validate_config(const SimConfig& cfg, std::string& error);

void print_usage(const char* program);
