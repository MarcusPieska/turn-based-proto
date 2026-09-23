#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    cities = []
    seat = None
    count = None
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                for part in line[1:].split():
                    if part.startswith("seat="):
                        seat = int(part.split("=", 1)[1])
                    elif part.startswith("count="):
                        count = int(part.split("=", 1)[1])
                continue
            if line.startswith("turn"):
                continue
            a, b = line.split()
            turns.append(int(a))
            cities.append(int(b))
    return seat, count, turns, cities

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_player_city_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    fig, ax = plt.subplots(figsize=(10, 6))
    for path in paths:
        seat, count, turns, cities = load_curve(path)
        label = path.stem
        if seat is not None and count is not None:
            label = f"seat {seat} (n={count})"
        ax.plot(turns, cities, linewidth=1.6, label=label)
    ax.set_xlabel("turn")
    ax.set_ylabel("cities founded")
    ax.set_title("Player city settle timelines")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "player_city_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
