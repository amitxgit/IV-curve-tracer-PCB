"""
Usage:
    python -m iv_tracer.cli --port /dev/ttyACM0 --start 0 --stop 3 --step 0.05 --dwell 10 --plot --save out.csv
"""

from __future__ import annotations
import argparse

from .sweep import run_iv_sweep
from .plotting import plot_iv, save_csv


def main():
    parser = argparse.ArgumentParser(description="Run an I-V sweep on the USB SCPI curve tracer.")
    parser.add_argument("--port", required=True, help="Serial port, e.g. COM5 or /dev/ttyACM0")
    parser.add_argument("--start", type=float, default=0.0)
    parser.add_argument("--stop", type=float, default=3.0)
    parser.add_argument("--step", type=float, default=0.05)
    parser.add_argument("--dwell", type=int, default=10, help="Dwell time per point, ms")
    parser.add_argument("--plot", action="store_true", help="Show the I-V plot")
    parser.add_argument("--save", type=str, default=None, help="Path to save CSV data")
    args = parser.parse_args()

    voltages, currents = run_iv_sweep(args.port, args.start, args.stop, args.step, args.dwell)
    print(f"Captured {len(voltages)} points.")

    if args.save:
        save_csv(args.save, voltages, currents)
        print(f"Saved to {args.save}")

    if args.plot:
        fig = plot_iv(voltages, currents, title=f"I-V Sweep ({args.start}V to {args.stop}V)")
        import matplotlib.pyplot as plt
        plt.show()


if __name__ == "__main__":
    main()
