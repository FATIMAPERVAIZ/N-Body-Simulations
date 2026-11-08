// MPI distributed direct O(N^2) N-body simulation.
//
// Idea: every process holds ALL bodies, but each process computes the new
// state of only its own slice of bodies. After every step the slices are
// shared with MPI_Allgatherv, so every process again has the full state.
//
// Extra: logs real communication data (bodies owned + comm time per rank)
//        into results/raw/mpi_comm_log.csv for visualization.

#include <mpi.h>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "configuration.hpp"
#include "initialization.hpp"
#include "io.hpp"
#include "physics.hpp"
#include "simulation.hpp"

// We send whole Body structs, so Body must be exactly 7 doubles with no padding.
static_assert(sizeof(Body) == 7 * sizeof(double), "Body must be 7 doubles");

// Balanced split of n bodies among p processes.
// counts[r] = how many bodies rank r owns
// displs[r] = index of its first body
static void make_partition(int n, int p, std::vector<int>& counts, std::vector<int>& displs) {
    counts.resize(p);
    displs.resize(p);
    const int base  = n / p;
    const int extra = n % p;
    for (int r = 0; r < p; ++r) {
        counts[r] = base + (r < extra ? 1 : 0);
        displs[r] = r * base + std::min(r, extra);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // ---- 1. Read and check parameters ----
    SimConfig cfg;
    cfg.output_file = "results/raw/final_mpi.csv";
    std::string error;

    const bool parsed = parse_config(argc, argv, cfg, error);
    if (parsed && cfg.show_help) {
        if (rank == 0) print_usage(argv[0]);
        MPI_Finalize();
        return 0;
    }
    if (!parsed || !validate_config(cfg, error)) {
        if (rank == 0) std::cerr << "Error: " << error << "\n";
        MPI_Finalize();
        return 1;
    }
    if (rank == 0 && cfg.snapshot_every > 0) {
        std::cerr << "Note: snapshots are not supported in the MPI program, option ignored.\n";
    }

    // ---- 2. MPI datatype for one Body (7 doubles) ----
    MPI_Datatype body_type;
    MPI_Type_contiguous(7, MPI_DOUBLE, &body_type);
    MPI_Type_commit(&body_type);

    // ---- 3. Initial state: rank 0 creates bodies, then broadcast ----
    const int n = cfg.bodies;
    std::vector<Body> current(n);
    std::vector<Body> next(n);
    if (rank == 0) current = create_bodies(cfg);
    MPI_Bcast(current.data(), n, body_type, 0, MPI_COMM_WORLD);

    // ---- 4. Data decomposition ----
    std::vector<int> counts, displs;
    make_partition(n, size, counts, displs);
    const int begin = displs[rank];
    const int end   = begin + counts[rank];

    // ---- 5. Time stepping ----
    double compute_s = 0.0;
    double comm_s    = 0.0;
    int failed_body  = -1;
    int failed_step  = 0;

    MPI_Barrier(MPI_COMM_WORLD);
    const double wall_start = MPI_Wtime();
    const std::clock_t cpu_start = std::clock();

    for (int step = 1; step <= cfg.steps; ++step) {
        // (a) Compute my slice
        const double t0 = MPI_Wtime();
        compute_step_range(current.data(), next.data(), n, begin, end, cfg);
        const double t1 = MPI_Wtime();

        // (b) Real communication: exchange updated slices
        MPI_Allgatherv(next.data() + begin, counts[rank], body_type,
                       current.data(), counts.data(), displs.data(), body_type,
                       MPI_COMM_WORLD);
        const double t2 = MPI_Wtime();

        // (c) Validity check on full state
        const int bad = find_invalid_body(current.data(), 0, n);
        const double t3 = MPI_Wtime();

        compute_s += (t1 - t0) + (t3 - t2);
        comm_s    += (t2 - t1);

        if (bad >= 0) {
            failed_body = bad;
            failed_step = step;
            break;
        }
    }

    const double wall_s = MPI_Wtime() - wall_start;
    const double cpu_s  = static_cast<double>(std::clock() - cpu_start) / CLOCKS_PER_SEC;

    // ---- 6. Gather timings from all ranks ----
    const double mine[4] = {compute_s, comm_s, cpu_s, wall_s};
    std::vector<double> all(rank == 0 ? 4 * size : 0);
    MPI_Gather(mine, 4, MPI_DOUBLE, all.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    // ---- 7. Real communication log (bodies owned + comm time per rank) ----
    struct CommInfo {
        int    rank;
        int    bodies_owned;
        double total_comm_time;
    };

    CommInfo my_info;
    my_info.rank             = rank;
    my_info.bodies_owned     = counts[rank];
    my_info.total_comm_time  = comm_s;

    std::vector<CommInfo> all_info(size);
    MPI_Gather(&my_info, sizeof(CommInfo), MPI_BYTE,
               all_info.data(), sizeof(CommInfo), MPI_BYTE,
               0, MPI_COMM_WORLD);

    MPI_Type_free(&body_type);

    int exit_code = 0;
    if (failed_body >= 0) {
        exit_code = 2;
        if (rank == 0) {
            std::cerr << "Error: invalid value (NaN or Inf) in body " << failed_body
                      << " at step " << failed_step
                      << ". Try a smaller --dt or a larger --radius.\n";
        }
    } else if (rank == 0) {
        // Max times across ranks
        double max_compute = 0, max_comm = 0, max_cpu = 0, max_wall = 0;
        for (int r = 0; r < size; ++r) {
            max_compute = std::max(max_compute, all[4 * r + 0]);
            max_comm    = std::max(max_comm,    all[4 * r + 1]);
            max_cpu     = std::max(max_cpu,     all[4 * r + 2]);
            max_wall    = std::max(max_wall,    all[4 * r + 3]);
        }

        // Save final state
        if (!save_state_csv(cfg.output_file, current, error)) {
            std::cerr << "Error: " << error << "\n";
            exit_code = 1;
        }

        // Write real communication log
        {
            std::ofstream log("results/raw/mpi_comm_log.csv");
            if (log) {
                log << "rank,bodies_owned,total_comm_time_s,total_bodies,processes\n";
                for (int r = 0; r < size; ++r) {
                    log << all_info[r].rank << ","
                        << all_info[r].bodies_owned << ","
                        << all_info[r].total_comm_time << ","
                        << n << "," << size << "\n";
                }
                std::cout << "Wrote real communication log: results/raw/mpi_comm_log.csv\n";
            }
        }

        // Human-readable summary
        std::cout << "MPI N body simulation\n"
                  << "  processes        : " << size << "\n"
                  << "  bodies           : " << cfg.bodies << "\n"
                  << "  steps            : " << cfg.steps << "\n"
                  << "  seed             : " << cfg.seed << "\n"
                  << "  compute time (s) : " << max_compute << "  (max over ranks)\n"
                  << "  comm time (s)    : " << max_comm    << "  (max over ranks)\n"
                  << "  total time (s)   : " << max_wall    << "  (max over ranks)\n"
                  << "  cpu time (s)     : " << max_cpu     << "  (max over ranks)\n"
                  << "  final state      : " << cfg.output_file << "\n";

        for (int r = 0; r < size; ++r) {
            std::cout << "RANK rank=" << r
                      << " bodies=" << counts[r]
                      << " compute_s=" << all[4 * r + 0]
                      << " comm_s=" << all[4 * r + 1]
                      << " cpu_s=" << all[4 * r + 2] << "\n";
        }

        std::cout << "RESULT mode=mpi procs=" << size
                  << " bodies=" << cfg.bodies
                  << " steps=" << cfg.steps
                  << " seed=" << cfg.seed
                  << " compute_s=" << max_compute
                  << " comm_s=" << max_comm
                  << " total_s=" << max_wall
                  << " cpu_s=" << max_cpu << std::endl;
    }

    MPI_Finalize();
    return exit_code;
}
