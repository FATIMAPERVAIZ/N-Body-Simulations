#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

df = pd.read_csv("results/raw/mpi_comm_log.csv")

fig, axes = plt.subplots(1, 2, figsize=(12, 5))

# 1. Bodies owned per rank (real load balance)
axes[0].bar(df["rank"], df["bodies_owned"], color="steelblue")
axes[0].set_xlabel("MPI Rank")
axes[0].set_ylabel("Bodies owned")
axes[0].set_title("Real Data Decomposition (bodies per rank)")
axes[0].grid(True, alpha=0.3)

# 2. Communication time per rank (real)
axes[1].bar(df["rank"], df["total_comm_time_s"], color="darkorange")
axes[1].set_xlabel("MPI Rank")
axes[1].set_ylabel("Total communication time (s)")
axes[1].set_title("Real Communication Time per Rank")
axes[1].grid(True, alpha=0.3)

plt.suptitle(f"Real MPI Communication Data  (N={df['total_bodies'].iloc[0]}, P={df['processes'].iloc[0]})")
plt.tight_layout()
Path("results/processed").mkdir(parents=True, exist_ok=True)
plt.savefig("results/processed/real_mpi_communication.png", dpi=150)
print("Saved results/processed/real_mpi_communication.png")
