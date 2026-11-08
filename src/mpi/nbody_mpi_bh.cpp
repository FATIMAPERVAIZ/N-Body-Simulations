#include <mpi.h>
#include <algorithm>
#include <ctime>
#include <iostream>
#include <vector>
#include <string>

#include "configuration.hpp"
#include "initialization.hpp"
#include "io.hpp"
#include "barnes_hut.hpp"
#include "physics.hpp"

static_assert(sizeof(Body) == 7 * sizeof(double), "Body layout");

static void make_partition(int n, int p, std::vector<int>& counts, std::vector<int>& displs) {
    counts.resize(p);
    displs.resize(p);
    int base = n / p, extra = n % p;
    for (int r = 0; r < p; ++r) {
        counts[r] = base + (r < extra ? 1 : 0);
        displs[r] = r * base + std::min(r, extra);
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    SimConfig cfg;
    cfg.output_file = "results/raw/final_mpi_bh.csv";
    std::string error;

    if (!parse_config(argc, argv, cfg, error) || !validate_config(cfg, error)) {
        if (rank == 0) std::cerr << "Error: " << error << "\n";
        MPI_Finalize();
        return 1;
    }

    MPI_Datatype body_type;
    MPI_Type_contiguous(7, MPI_DOUBLE, &body_type);
    MPI_Type_commit(&body_type);

    const int n = cfg.bodies;
    std::vector<Body> current(n), next(n);
    if (rank == 0) current = create_bodies(cfg);
    MPI_Bcast(current.data(), n, body_type, 0, MPI_COMM_WORLD);

    std::vector<int> counts, displs;
    make_partition(n, size, counts, displs);
    const int begin = displs[rank];
    const int end   = begin + counts[rank];

    double compute_s = 0.0, comm_s = 0.0;

    MPI_Barrier(MPI_COMM_WORLD);
    double wall_start = MPI_Wtime();
    std::clock_t cpu_start = std::clock();

    std::vector<double> ax, ay;

    for (int step = 1; step <= cfg.steps; ++step) {
        double t0 = MPI_Wtime();

        // Barnes-Hut on the full current state
        // (each rank computes accelerations only for its own slice)
        barnes_hut_accelerations(current, ax, ay, cfg, 0.5);

        for (int i = begin; i < end; ++i) {
            Body& b = next[i];
            b = current[i];
            b.ax = ax[i];
            b.ay = ay[i];
            b.vx += b.ax * cfg.dt;
            b.vy += b.ay * cfg.dt;
            b.x  += b.vx * cfg.dt;
            b.y  += b.vy * cfg.dt;
            apply_wall_collision(b, cfg);
        }
        double t1 = MPI_Wtime();

        // Exchange updated slices
        MPI_Allgatherv(next.data() + begin, counts[rank], body_type,
                       current.data(), counts.data(), displs.data(), body_type,
                       MPI_COMM_WORLD);
        double t2 = MPI_Wtime();

        compute_s += (t1 - t0);
        comm_s    += (t2 - t1);
    }

    double wall_s = MPI_Wtime() - wall_start;
    double cpu_s  = double(std::clock() - cpu_start) / CLOCKS_PER_SEC;

    double mine[4] = {compute_s, comm_s, cpu_s, wall_s};
    std::vector<double> all(rank == 0 ? 4 * size : 0);
    MPI_Gather(mine, 4, MPI_DOUBLE, all.data(), 4, MPI_DOUBLE, 0, MPI_COMM_WORLD);

    MPI_Type_free(&body_type);

    if (rank == 0) {
        double max_compute = 0, max_comm = 0, max_wall = 0;
        for (int r = 0; r < size; ++r) {
            max_compute = std::max(max_compute, all[4*r + 0]);
            max_comm    = std::max(max_comm,    all[4*r + 1]);
            max_wall    = std::max(max_wall,    all[4*r + 3]);
        }

        save_state_csv(cfg.output_file, current, error);

        std::cout << "MPI Barnes-Hut N-body\n"
                  << "  processes        : " << size << "\n"
                  << "  bodies           : " << n << "\n"
                  << "  compute time (s) : " << max_compute << "\n"
                  << "  comm time (s)    : " << max_comm << "\n"
                  << "  total time (s)   : " << max_wall << "\n";

        std::cout << "RESULT mode=mpi_bh procs=" << size
                  << " bodies=" << n
                  << " compute_s=" << max_compute
                  << " comm_s=" << max_comm
                  << " total_s=" << max_wall << std::endl;
    }

    MPI_Finalize();
    return 0;
}
