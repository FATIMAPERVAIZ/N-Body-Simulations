#!/usr/bin/env python3
import argparse
from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_CSV = ROOT / "results" / "processed" / "experiment_results.csv"
OUT_DIR = ROOT / "results" / "processed"

def load(csv_path):
    df = pd.read_csv(csv_path)
    for col in ["procs","bodies","steps","compute_s","comm_s","total_s","cpu_s",
                "max_bodies_per_rank","min_bodies_per_rank"]:
        if col in df.columns:
            df[col] = pd.to_numeric(df[col], errors="coerce")
    return df

def add_metrics(df):
    seq = df[df["mode"]=="sequential"].set_index("bodies")["total_s"]
    mpi = df[df["mode"]=="mpi"].copy()
    mpi["T1"] = mpi["bodies"].map(seq)
    mpi["speedup"] = mpi["T1"] / mpi["total_s"]
    mpi["efficiency"] = mpi["speedup"] / mpi["procs"]
    mpi["comm_fraction"] = mpi["comm_s"] / mpi["total_s"]
    return mpi

def plot_speedup(mpi, out):
    fig, ax = plt.subplots(figsize=(8,5))
    for n, g in mpi.groupby("bodies"):
        g = g.sort_values("procs")
        ax.plot(g["procs"], g["speedup"], marker="o", label=f"N={n}")
    procs = sorted(mpi["procs"].unique())
    ax.plot(procs, procs, "--", color="gray", label="ideal")
    ax.set_xlabel("Processes"); ax.set_ylabel("Speedup"); ax.set_title("Speedup")
    ax.legend(); ax.grid(True, alpha=0.3); fig.tight_layout(); fig.savefig(out, dpi=150); plt.close()
    print("Wrote", out)

def plot_efficiency(mpi, out):
    fig, ax = plt.subplots(figsize=(8,5))
    for n, g in mpi.groupby("bodies"):
        g = g.sort_values("procs")
        ax.plot(g["procs"], g["efficiency"], marker="o", label=f"N={n}")
    ax.axhline(1.0, ls="--", color="gray")
    ax.set_xlabel("Processes"); ax.set_ylabel("Efficiency"); ax.set_title("Efficiency")
    ax.set_ylim(0, 1.15); ax.legend(); ax.grid(True, alpha=0.3)
    fig.tight_layout(); fig.savefig(out, dpi=150); plt.close(); print("Wrote", out)

def plot_comm(mpi, out):
    fig, ax = plt.subplots(figsize=(8,5))
    for n, g in mpi.groupby("bodies"):
        g = g.sort_values("procs")
        ax.plot(g["procs"], g["comm_fraction"], marker="o", label=f"N={n}")
    ax.set_xlabel("Processes"); ax.set_ylabel("Comm fraction"); ax.set_title("Communication overhead")
    ax.legend(); ax.grid(True, alpha=0.3); fig.tight_layout(); fig.savefig(out, dpi=150); plt.close()
    print("Wrote", out)

def plot_scalability(mpi, out):
    fig, ax = plt.subplots(figsize=(8,5))
    for p, g in mpi.groupby("procs"):
        g = g.sort_values("bodies")
        ax.plot(g["bodies"], g["total_s"], marker="o", label=f"P={p}")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xlabel("Bodies N"); ax.set_ylabel("Total time (s)"); ax.set_title("Scalability")
    ax.legend(); ax.grid(True, which="both", alpha=0.3)
    fig.tight_layout(); fig.savefig(out, dpi=150); plt.close(); print("Wrote", out)

def plot_load(mpi, out):
    fig, ax = plt.subplots(figsize=(8,5))
    for n, g in mpi.groupby("bodies"):
        g = g.sort_values("procs")
        ax.plot(g["procs"], g["max_bodies_per_rank"], marker="o", label=f"N={n} max")
        ax.plot(g["procs"], g["min_bodies_per_rank"], marker="x", ls="--", label=f"N={n} min")
    ax.set_xlabel("Processes"); ax.set_ylabel("Bodies per rank"); ax.set_title("Load balance")
    ax.legend(fontsize=8); ax.grid(True, alpha=0.3)
    fig.tight_layout(); fig.savefig(out, dpi=150); plt.close(); print("Wrote", out)

def main():
    p = argparse.ArgumentParser()
    p.add_argument("--csv", type=Path, default=DEFAULT_CSV)
    p.add_argument("--out-dir", type=Path, default=OUT_DIR)
    args = p.parse_args()
    if not args.csv.exists():
        raise SystemExit(f"CSV not found: {args.csv}\nRun scripts/run_experiments.sh first.")
    args.out_dir.mkdir(parents=True, exist_ok=True)
    df = load(args.csv)
    mpi = add_metrics(df)
    if mpi.empty: raise SystemExit("No MPI rows")
    plot_speedup(mpi, args.out_dir / "speedup.png")
    plot_efficiency(mpi, args.out_dir / "efficiency.png")
    plot_comm(mpi, args.out_dir / "comm_fraction.png")
    plot_scalability(mpi, args.out_dir / "scalability.png")
    plot_load(mpi, args.out_dir / "load_balance.png")
    cols = ["bodies","procs","total_s","compute_s","comm_s","speedup","efficiency","comm_fraction",
            "max_bodies_per_rank","min_bodies_per_rank"]
    mpi[cols].sort_values(["bodies","procs"]).to_csv(args.out_dir/"summary_metrics.csv", index=False, float_format="%.6g")
    print("Done. Figures in", args.out_dir)

if __name__ == "__main__":
    main()
