from __future__ import annotations
import csv
import numpy as np


def plot_iv(voltages: np.ndarray, currents: np.ndarray, title: str = "I-V Curve", fitted=None):
    import matplotlib.pyplot as plt  # lazy: only needed when actually plotting
    fig, ax = plt.subplots(figsize=(7, 5))
    ax.plot(voltages, currents * 1000, "o", markersize=3, label="Measured")
    if fitted is not None:
        ax.plot(voltages, fitted * 1000, "-", label="Shockley fit")
    ax.set_xlabel("Voltage (V)")
    ax.set_ylabel("Current (mA)")
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    return fig


def save_csv(path: str, voltages: np.ndarray, currents: np.ndarray):
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["voltage_V", "current_A"])
        for v, i in zip(voltages, currents):
            writer.writerow([f"{v:.4f}", f"{i:.6f}"])
