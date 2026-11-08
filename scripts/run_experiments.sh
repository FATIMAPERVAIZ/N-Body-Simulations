#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${ROOT}/build"
SEQ="${BUILD}/nbody_seq"
MPI="${BUILD}/nbody_mpi"
LOGDIR="${ROOT}/results/experiment_logs"
RAW="${ROOT}/results/raw"
OUT_CSV="${ROOT}/results/processed/experiment_results.csv"

BODIES_LIST=(100 500 1000 5000 10000)
PROCS_LIST=(1 2 4 8)
STEPS=100
SEED=42
DT=0.02
QUICK=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --quick)   QUICK=1; shift ;;
    --steps)   STEPS="$2"; shift 2 ;;
    --seed)    SEED="$2"; shift 2 ;;
    --dt)      DT="$2"; shift 2 ;;
    --bodies)  IFS=' ' read -r -a BODIES_LIST <<< "$2"; shift 2 ;;
    --procs)   IFS=' ' read -r -a PROCS_LIST <<< "$2"; shift 2 ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
done

if [[ "$QUICK" -eq 1 ]]; then
  BODIES_LIST=(100 500 1000)
  PROCS_LIST=(1 2 4)
  STEPS=50
fi

mkdir -p "$LOGDIR" "$RAW" "$(dirname "$OUT_CSV")"
if [[ ! -x "$SEQ" || ! -x "$MPI" ]]; then
  echo "Error: build first (mkdir build && cd build && cmake .. && make -j)"
  exit 1
fi

echo "mode,procs,bodies,steps,seed,compute_s,comm_s,total_s,cpu_s,max_bodies_per_rank,min_bodies_per_rank" > "$OUT_CSV"
timestamp=$(date +%Y%m%d_%H%M%S)
LOG="${LOGDIR}/run_${timestamp}.log"
exec > >(tee -a "$LOG") 2>&1

echo "=== Experiments ==="
echo "Bodies: ${BODIES_LIST[*]}   Procs: ${PROCS_LIST[*]}   Steps: $STEPS"
echo

run_seq() {
  local n=$1
  local out="${RAW}/final_seq_n${n}_s${STEPS}.csv"
  echo "---- Sequential  bodies=$n ----"
  "$SEQ" --bodies "$n" --steps "$STEPS" --seed "$SEED" --dt "$DT" --output "$out" | tee /tmp/nbody_seq_out.txt
  local line compute total
  line=$(grep '^RESULT ' /tmp/nbody_seq_out.txt | tail -1)
  compute=$(echo "$line" | sed -n 's/.*compute_s=\([0-9.eE+-]*\).*/\1/p')
  total=$(echo "$line" | sed -n 's/.*total_s=\([0-9.eE+-]*\).*/\1/p')
  echo "sequential,1,$n,$STEPS,$SEED,$compute,0,$total,$total,$n,$n" >> "$OUT_CSV"
}

run_mpi() {
  local n=$1 p=$2
  local out="${RAW}/final_mpi_n${n}_p${p}_s${STEPS}.csv"
  echo "---- MPI  bodies=$n  procs=$p ----"
  mpirun -np "$p" "$MPI" --bodies "$n" --steps "$STEPS" --seed "$SEED" --dt "$DT" --output "$out" | tee /tmp/nbody_mpi_out.txt
  local line compute comm total cpu
  line=$(grep '^RESULT ' /tmp/nbody_mpi_out.txt | tail -1)
  compute=$(echo "$line" | sed -n 's/.*compute_s=\([0-9.eE+-]*\).*/\1/p')
  comm=$(echo "$line" | sed -n 's/.*comm_s=\([0-9.eE+-]*\).*/\1/p')
  total=$(echo "$line" | sed -n 's/.*total_s=\([0-9.eE+-]*\).*/\1/p')
  cpu=$(echo "$line" | sed -n 's/.*cpu_s=\([0-9.eE+-]*\).*/\1/p')
  local max_b=0 min_b=999999999
  while read -r rline; do
    local b; b=$(echo "$rline" | sed -n 's/.*bodies=\([0-9]*\).*/\1/p')
    [[ -n "$b" ]] && { (( b > max_b )) && max_b=$b; (( b < min_b )) && min_b=$b; }
  done < <(grep '^RANK ' /tmp/nbody_mpi_out.txt)
  echo "mpi,$p,$n,$STEPS,$SEED,$compute,$comm,$total,$cpu,$max_b,$min_b" >> "$OUT_CSV"
}

for n in "${BODIES_LIST[@]}"; do run_seq "$n"; done
for n in "${BODIES_LIST[@]}"; do
  for p in "${PROCS_LIST[@]}"; do run_mpi "$n" "$p"; done
done

echo
echo "Results CSV : $OUT_CSV"
echo "Full log    : $LOG"
echo "Next: python3 scripts/plot_results.py"
