#!/usr/bin/env python3
"""
Educational visualization:
- Bodies colored by owning MPI rank
- Temporary lines drawn to suggest Allgatherv communication
"""

import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, PillowWriter
from pathlib import Path

def assign_ranks(n_bodies, n_procs):
    base = n_bodies // n_procs
    extra = n_bodies % n_procs
    ranks = np.zeros(n_bodies, dtype=int)
    start = 0
    for r in range(n_procs):
        count = base + (1 if r < extra else 0)
        ranks[start:start + count] = r
        start += count
    return ranks

def rank_centers(ranks, x, y, n_procs):
    """Average position of bodies belonging to each rank (for drawing messages)"""
    centers = []
    for r in range(n_procs):
        mask = ranks == r
        if np.any(mask):
            centers.append((x[mask].mean(), y[mask].mean()))
        else:
            centers.append((0.0, 0.0))
    return centers

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", help="final state CSV")
    parser.add_argument("--procs", type=int, default=4)
    parser.add_argument("--space", type=float, default=800.0)
    parser.add_argument("--frames", type=int, default=40, help="number of animation frames")
    parser.add_argument("--save", type=str, default="results/processed/messages.gif")
    args = parser.parse_args()

    df = pd.read_csv(args.csv)
    x = df["x"].values
    y = df["y"].values
    n = len(df)
    ranks = assign_ranks(n, args.procs)
    centers = rank_centers(ranks, x, y, args.procs)

    fig, ax = plt.subplots(figsize=(8, 8))
    ax.set_xlim(0, args.space)
    ax.set_ylim(0, args.space)
    ax.set_aspect("equal")
    ax.set_title(f"N-Body + educational view of Allgatherv  (N={n}, P={args.procs})")

    scatter = ax.scatter(x, y, c=ranks, cmap="tab10", s=18, alpha=0.85)
    plt.colorbar(scatter, ax=ax, label="Owning MPI Rank")

    # lines that will represent messages
    lines = []
    for i in range(args.procs):
        for j in range(i + 1, args.procs):
            (line,) = ax.plot([], [], "k-", alpha=0.0, lw=1)
            lines.append((line, i, j))

    def init():
        for line, _, _ in lines:
            line.set_data([], [])
            line.set_alpha(0.0)
        return [l[0] for l in lines]

    def update(frame):
        # Make messages appear and fade (simulates periodic Allgatherv)
        phase = (frame % 20) / 20.0
        alpha = max(0.0, 1.0 - abs(phase - 0.5) * 2.5)  # peak in the middle

        for line, i, j in lines:
            xdata = [centers[i][0], centers[j][0]]
            ydata = [centers[i][1], centers[j][1]]
            line.set_data(xdata, ydata)
            line.set_alpha(alpha * 0.6)
        return [l[0] for l in lines]

    anim = FuncAnimation(fig, update, init_func=init,
                         frames=args.frames, interval=120, blit=True)

    out = Path(args.save)
    out.parent.mkdir(parents=True, exist_ok=True)

    print("Saving animation (this may take a few seconds)...")
    anim.save(out, writer=PillowWriter(fps=8))
    print("Saved:", out)

if __name__ == "__main__":
    main()
