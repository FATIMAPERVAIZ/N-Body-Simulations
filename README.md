# Distributed N-Body Simulation (MPI)

**CT-488 Distributed Computing – Complex Computing Activity (CCA)**

A complete sequential and distributed (MPI) N-Body simulation with optional Barnes–Hut optimization, performance measurement, correctness verification, and visualization.

## 1. Project Overview

This project implements the classical gravitational N-Body problem in two forms:

1. **Sequential direct O(N²)** method – used as the correctness and performance baseline.
2. **Distributed MPI version** – bodies are partitioned among processes, forces are computed locally, and state is exchanged every step with `MPI_Allgatherv`.

An optional **Barnes–Hut** hierarchical approximation (O(N log N)) is also provided for both sequential and MPI execution, allowing simulations with up to 100 000 bodies.

The implementation demonstrates:

- Balanced data decomposition (handles `N % P ≠ 0`)
- Collective MPI communication and implicit synchronization
- Load-balance evaluation
- Separate measurement of computation and communication time
- Numerical agreement between sequential and MPI results
- Scalability, speedup and efficiency analysis
- Supporting visualizations

---

## 2. Features

| Feature | Status |
|---------|--------|
| Sequential direct O(N²) | ✅ |
| MPI distributed version (1, 2, 4, 8+ processes) | ✅ |
| Balanced partitioning for uneven N | ✅ |
| MPI_Bcast + MPI_Allgatherv | ✅ |
| Computation / Communication / Total timing | ✅ |
| Correctness verification (tolerance 1e-6) | ✅ |
| Experiment driver + CSV output | ✅ |
| Performance graphs (speedup, efficiency, etc.) | ✅ |
| Sequential Barnes–Hut | ✅ |
| MPI Barnes–Hut | ✅ |
| Real-time Raylib visualization | ✅ |
| Rank-coloured body plots & communication charts | ✅ |

---

## 3. Requirements

### System packages (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install -y build-essential cmake libopenmpi-dev openmpi-bin \
                    python3 python3-venv python3-pip \
                    libraylib-dev   # optional, for real-time visualization
```

### Python packages (for plotting)

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install matplotlib pandas pillow
```

---

## 4. Building the Project

```bash
cd distributed_nbody_simulation
mkdir -p build
cd build
cmake ..
make -j$(nproc)
cd ..
```

This produces the following executables inside `build/`:

| Executable | Description |
|------------|-------------|
| `nbody_seq` | Sequential direct O(N²) |
| `nbody_mpi` | MPI direct method |
| `nbody_seq_bh` | Sequential Barnes–Hut |
| `nbody_mpi_bh` | MPI Barnes–Hut |
| `compare_states` | Sequential vs MPI state comparison |
| `test_physics` | Unit tests for the physics kernel |

---

## 5. Running the Programs

### 5.1 Sequential Direct Method

```bash
./build/nbody_seq --bodies 1000 --steps 100 --seed 42 --output results/raw/final_seq.csv
```

### 5.2 MPI Direct Method

```bash
mpirun -np 4 ./build/nbody_mpi --bodies 1000 --steps 100 --seed 42 \
       --output results/raw/final_mpi.csv
```

### 5.3 Sequential Barnes–Hut

```bash
./build/nbody_seq_bh --bodies 20000 --steps 30 --seed 42
```

### 5.4 MPI Barnes–Hut

```bash
mpirun -np 8 ./build/nbody_mpi_bh --bodies 100000 --steps 15 --seed 42
```

### Common Command-Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--bodies N` | Number of bodies | 1000 |
| `--steps S` | Number of time steps | 100 |
| `--seed S` | Random seed for initial conditions | 42 |
| `--dt D` | Time-step size | 0.02 |
| `--output FILE` | Path for final state CSV | results/raw/... |

---

## 6. Correctness Verification (Stage 6)

Compare sequential and MPI final states within a numerical tolerance:

```bash
bash scripts/verify_correctness.sh 500 4 100 42 1e-6
```

Or manually:

```bash
./build/nbody_seq --bodies 500 --steps 100 --seed 42 --output results/raw/verify_seq.csv
mpirun -np 4 ./build/nbody_mpi --bodies 500 --steps 100 --seed 42 --output results/raw/verify_mpi.csv
./build/compare_states results/raw/verify_seq.csv results/raw/verify_mpi.csv --tol 1e-6
```

Expected output: `PASS: states agree within tolerance`

---

## 7. Running the Full Experiment Matrix (Stage 8)

```bash
source .venv/bin/activate
bash scripts/run_experiments.sh          # full matrix (N up to 10 000)
# or
bash scripts/run_experiments.sh --quick  # smaller matrix for testing
```

Results are written to:

- `results/processed/experiment_results.csv`
- `results/experiment_logs/`

---

## 8. Generating Performance Graphs (Stage 9)

```bash
source .venv/bin/activate
python scripts/plot_results.py
```

This creates the following files inside `results/processed/`:

| File | Description |
|------|-------------|
| `speedup.png` | Speedup vs number of processes |
| `efficiency.png` | Parallel efficiency |
| `comm_fraction.png` | Communication time fraction |
| `scalability.png` | Time vs number of bodies |
| `load_balance.png` | Max/min bodies per rank |
| `summary_metrics.csv` | Derived metrics table |

---

## 9. Visualizations

### Rank-coloured body plot

```bash
python scripts/visualize.py results/raw/final_mpi_n1000_p4_s50.csv --procs 4 \
       --save results/processed/bodies_by_rank.png
```

### Real communication data

```bash
python scripts/visualize_real_comm.py
# produces results/processed/real_mpi_communication.png
```

### Educational message animation (GIF)

```bash
python scripts/visualize_messages.py results/raw/final_mpi_n1000_p4_s50.csv \
       --procs 4 --save results/processed/messages.gif
```

### Real-time Raylib demo

```bash
# (requires libraylib-dev)
g++ -O2 -o nbody_raylib src/visualization/nbody_raylib.cpp \
    -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
./nbody_raylib
```

Controls: `SPACE` = pause/resume, `R` = reset, `ESC` = quit.

---

## 10. Project Structure

```
distributed_nbody_simulation/
├── CMakeLists.txt
├── README.md
├── include/
│   ├── body.hpp
│   ├── configuration.hpp
│   ├── physics.hpp
│   ├── simulation.hpp
│   ├── initialization.hpp
│   ├── io.hpp
│   └── barnes_hut.hpp
├── src/
│   ├── common/          # shared physics, I/O, Barnes-Hut tree
│   ├── sequential/      # nbody_seq.cpp, nbody_seq_bh.cpp
│   ├── mpi/             # nbody_mpi.cpp, nbody_mpi_bh.cpp
│   └── visualization/   # nbody_raylib.cpp
├── tools/
│   └── compare_states.cpp
├── scripts/
│   ├── verify_correctness.sh
│   ├── run_experiments.sh
│   ├── plot_results.py
│   ├── visualize.py
│   ├── visualize_real_comm.py
│   └── visualize_messages.py
├── tests/
│   └── test_physics.cpp
└── results/
    ├── raw/             # final state CSVs, communication logs
    ├── processed/       # graphs and summary tables
    └── experiment_logs/ # full terminal logs of experiment runs
```

---

## 11. Reproducibility Notes

- All runs use the same random seed (`--seed 42`) so sequential and MPI start from identical initial conditions.
- Compiler flags deliberately avoid `-ffast-math` and `-march=native` to keep floating-point results reproducible across machines.
- Timing is performed with `MPI_Wtime` (wall-clock) and `std::clock` (CPU).
- The maximum value across ranks is reported for compute, communication and total time, giving a conservative view of parallel performance.
- Numerical tolerance for state comparison is set to `1e-6`. Small residual differences are expected due to floating-point associativity.

---

## 12. Example Performance Snapshot

**Direct method (N = 1000, P = 4, 50 steps)**

| Metric | Value |
|--------|-------|
| Max compute time | 0.0779 s |
| Max communication time | 0.0456 s |
| Max total time | 0.0804 s |
| Bodies per rank | 250 (perfectly balanced) |

**Barnes–Hut**

| Configuration | N | P | Steps | Compute (s) | Total (s) |
|---------------|---|---|-------|-------------|-----------|
| Sequential BH | 20 000 | 1 | 30 | ≈ 1.65 | ≈ 1.71 |
| Sequential BH | 100 000 | 1 | 15 | 5.33 | 5.62 |
| MPI BH | 50 000 | 8 | 20 | 10.83 | 11.61 |
| MPI BH | 100 000 | 8 | 15 | 13.94 | 15.95 |

---

## 13. License & Acknowledgements

This project was developed as a Complex Computing Activity for the course CT-488 Distributed Computing.

Barnes–Hut tree code follows the classic algorithm of Barnes & Hut (1986).  
Raylib is used under its zlib license for the optional real-time visualizer.
