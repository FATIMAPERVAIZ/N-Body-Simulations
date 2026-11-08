#!/usr/bin/env python3
"""
Simple visualization of N-Body results.
- Reads a final-state CSV or a snapshots CSV
- Colors bodies by which MPI rank would own them (shows data decomposition)
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation
import pandas as pd
from pathlib import Path

def load_final_csv(path):
    df = pd.read_csv(path)
    return df

def assign_ranks(n_bodies, n_procs):
    """Same balanced partition formula used in the C++ code"""
    base = n_bodies // n_procs
    extra = n_bodies % n_procs
    ranks = np.zeros(n_bodies, dtype=int)
    start = 0
    for r in range(n_procs):
        count = base + (1 if r < extra else 0)
        ranks[start:start+count] = r
        start += count
    return ranks

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="final state CSV (id,x,y,vx,vy,m)")
    parser.add_argument("--procs", type=int, default=4, help="number of MPI ranks to color by")
    parser.add_argument("--space", type=float, default=800.0)
    parser.add_argument("--save", type=str, default=None, help="save image instead of showing")
    args = parser.parse_args()

    df = load_final_csv(args.csv)
    n = len(df)
    ranks = assign_ranks(n, args.procs)

    plt.figure(figsize=(8, 8))
    scatter = plt.scatter(df["x"], df["y"], c=ranks, cmap="tab10", s=20, alpha=0.8)
    plt.colorbar(scatter, label=f"MPI Rank (P={args.procs})")
    plt.xlim(0, args.space)
    plt.ylim(0, args.space)
    plt.gca().set_aspect("equal")
    plt.title(f"N-Body final state  (N={n}, colored by owning rank)")
    plt.xlabel("x")
    plt.ylabel("y")
    plt.tight_layout()

    if args.save:
        plt.savefig(args.save, dpi=150)
        print("Saved", args.save)
    else:
        plt.show()

if __name__ == "__main__":
    main()
