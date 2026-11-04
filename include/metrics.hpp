#pragma once
#include <chrono>

// Simple wall clock stopwatch for the sequential program.
// Stage 7 adds MPI_Wtime based timing for the MPI program.
class Stopwatch {
    using clock = std::chrono::steady_clock;

public:
    void start() { start_ = clock::now(); }

    double elapsed_seconds() const {
        return std::chrono::duration<double>(clock::now() - start_).count();
    }

private:
    clock::time_point start_ = clock::now();
};
