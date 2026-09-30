#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    avgs = []
    maxes = []
    seat = None
    land = None
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                for part in line[1:].split():
                    if part.startswith("seat="):
                        seat = int(part.split("=", 1)[1])
                    elif part.startswith("land="):
                        land = int(part.split("=", 1)[1])
                continue
            if line.startswith("turn"):
                continue
            a, b, c = line.split()
            turns.append(int(a))
            avgs.append(float(b))
            maxes.append(float(c))
    return seat, land, turns, avgs, maxes

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_player_land_size_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    fig, ax = plt.subplots(figsize=(10, 6))
    for path in paths:
        seat, land, turns, avgs, maxes = load_curve(path)
        label = path.stem
        if seat is not None and land is not None:
            label = f"seat {seat} (land={land})"
        line, = ax.plot(turns, avgs, linewidth=2.4, label=label)
        ax.plot(turns, maxes, linewidth=2.4, color=line.get_color(), linestyle="--")
    ax.set_xlabel("turn")
    ax.set_ylabel("size (1 + raw)")
    ax.set_title("Player land unit size: avg (solid) vs max (dashed)")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "player_land_size_save_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
