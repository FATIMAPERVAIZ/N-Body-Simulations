#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
BODIES=${1:-500}
PROCS=${2:-4}
STEPS=${3:-100}
SEED=${4:-42}
TOL=${5:-1e-6}

SEQ_OUT="${ROOT}/results/raw/verify_seq_n${BODIES}.csv"
MPI_OUT="${ROOT}/results/raw/verify_mpi_n${BODIES}_p${PROCS}.csv"
mkdir -p "${ROOT}/results/raw"

echo "Running sequential (N=$BODIES) ..."
"$BUILD/nbody_seq" --bodies "$BODIES" --steps "$STEPS" --seed "$SEED" --output "$SEQ_OUT"

echo "Running MPI (P=$PROCS) ..."
mpirun -np "$PROCS" "$BUILD/nbody_mpi" --bodies "$BODIES" --steps "$STEPS" --seed "$SEED" --output "$MPI_OUT"

echo "Comparing (tol=$TOL) ..."
"$BUILD/compare_states" "$SEQ_OUT" "$MPI_OUT" --tol "$TOL"
