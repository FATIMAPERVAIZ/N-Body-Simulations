// Sequential direct O(N^2) N body simulation. This is the correctness and
// performance baseline for the MPI version.
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "configuration.hpp"
#include "initialization.hpp"
#include "io.hpp"
#include "metrics.hpp"
#include "physics.hpp"
#include "simulation.hpp"

int main(int argc, char** argv) {
    SimConfig cfg;
    cfg.output_file = "results/raw/final_seq.csv";
    cfg.snapshot_file = "results/raw/snapshots_seq.csv";

    std::string error;
    if (!parse_config(argc, argv, cfg, error)) {
        std::cerr << "Error: " << error << "\n\n";
        print_usage(argv[0]);
        return 1;
    }
    if (cfg.show_help) {
        print_usage(argv[0]);
        return 0;
    }
    if (!validate_config(cfg, error)) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }

    Stopwatch total_timer;
    total_timer.start();

    std::vector<Body> current = create_bodies(cfg);
    std::vector<Body> next(current.size());
    const int n = static_cast<int>(current.size());

    std::ofstream snapshots;
    if (cfg.snapshot_every > 0) {
        if (!open_snapshot_file(cfg.snapshot_file, snapshots, error)) {
            std::cerr << "Error: " << error << "\n";
            return 1;
        }
        append_snapshot(snapshots, 0, current);
    }

    double compute_seconds = 0.0;
    Stopwatch step_timer;

    for (int step = 1; step <= cfg.steps; ++step) {
        // Only the physics and the validity check are counted as computation.
        step_timer.start();
        step_sequential(current, next, cfg);
        const int bad = find_invalid_body(next.data(), 0, n);
        compute_seconds += step_timer.elapsed_seconds();

        if (bad >= 0) {
            std::cerr << "Error: invalid value (NaN or Inf) in body " << bad << " at step "
                      << step << ". Try a smaller --dt or a larger --radius.\n";
            return 2;
        }

        current.swap(next);  // new state becomes the snapshot for the next step

        if (cfg.snapshot_every > 0 && step % cfg.snapshot_every == 0) {
            append_snapshot(snapshots, step, current);
        }
    }

    if (!save_state_csv(cfg.output_file, current, error)) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }

    const double total_seconds = total_timer.elapsed_seconds();

    std::cout << "Sequential N body simulation\n"
              << "  bodies           : " << cfg.bodies << "\n"
              << "  steps            : " << cfg.steps << "\n"
              << "  seed             : " << cfg.seed << "\n"
              << "  compute time (s) : " << compute_seconds << "\n"
              << "  total time (s)   : " << total_seconds << "\n"
              << "  final state      : " << cfg.output_file << "\n";
    std::cout << "RESULT mode=sequential bodies=" << cfg.bodies << " steps=" << cfg.steps
              << " seed=" << cfg.seed << " compute_s=" << compute_seconds
              << " total_s=" << total_seconds << std::endl;
    return 0;
}
