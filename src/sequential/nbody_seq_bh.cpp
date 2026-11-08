#include <iostream>
#include <vector>
#include <string>

#include "configuration.hpp"
#include "initialization.hpp"
#include "io.hpp"
#include "metrics.hpp"
#include "barnes_hut.hpp"
#include "physics.hpp"   // for wall collisions + find_invalid_body

int main(int argc, char** argv) {
    SimConfig cfg;
    cfg.output_file = "results/raw/final_seq_bh.csv";

    std::string error;
    if (!parse_config(argc, argv, cfg, error)) {
        std::cerr << "Error: " << error << "\n";
        print_usage(argv[0]);
        return 1;
    }
    if (cfg.show_help) { print_usage(argv[0]); return 0; }
    if (!validate_config(cfg, error)) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }

    Stopwatch total_timer;
    total_timer.start();

    std::vector<Body> bodies = create_bodies(cfg);
    const int n = static_cast<int>(bodies.size());
    std::vector<double> ax, ay;

    double compute_seconds = 0.0;
    Stopwatch step_timer;

    for (int step = 1; step <= cfg.steps; ++step) {
        step_timer.start();

        // Barnes-Hut accelerations
        barnes_hut_accelerations(bodies, ax, ay, cfg, 0.5);

        // Integrate (semi-implicit Euler) + wall collisions
        for (int i = 0; i < n; ++i) {
            Body& b = bodies[i];
            b.ax = ax[i];
            b.ay = ay[i];
            b.vx += b.ax * cfg.dt;
            b.vy += b.ay * cfg.dt;
            b.x  += b.vx * cfg.dt;
            b.y  += b.vy * cfg.dt;
            apply_wall_collision(b, cfg);
        }

        compute_seconds += step_timer.elapsed_seconds();

        if (find_invalid_body(bodies.data(), 0, n) >= 0) {
            std::cerr << "NaN/Inf detected at step " << step << "\n";
            return 2;
        }
    }

    if (!save_state_csv(cfg.output_file, bodies, error)) {
        std::cerr << "Error: " << error << "\n";
        return 1;
    }

    const double total_seconds = total_timer.elapsed_seconds();

    std::cout << "Sequential Barnes-Hut N-body\n"
              << "  bodies           : " << cfg.bodies << "\n"
              << "  steps            : " << cfg.steps << "\n"
              << "  compute time (s) : " << compute_seconds << "\n"
              << "  total time (s)   : " << total_seconds << "\n"
              << "  final state      : " << cfg.output_file << "\n";

    std::cout << "RESULT mode=sequential_bh bodies=" << cfg.bodies
              << " steps=" << cfg.steps
              << " compute_s=" << compute_seconds
              << " total_s=" << total_seconds << std::endl;

    return 0;
}
