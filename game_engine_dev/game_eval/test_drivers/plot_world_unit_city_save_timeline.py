#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

U16_MAX = 65535
Y_LIM = 70000

def load_totals(path: Path):
    turns = []
    units = []
    cities = []
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("turn"):
                continue
            a, b, c = line.split()
            turns.append(int(a))
            units.append(int(b))
            cities.append(int(c))
    return turns, units, cities

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_world_unit_city_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    path = data_dir / "totals.txt"
    if not path.is_file():
        print(f"missing {path}")
        return 1
    turns, units, cities = load_totals(path)
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(turns, units, linewidth=1.8, label="units")
    ax.plot(turns, cities, linewidth=1.8, label="cities")
    ax.axhline(U16_MAX, color="#888888", linewidth=3.0, label="u16 max")
    ax.set_ylim(0, Y_LIM)
    ax.set_xlabel("turn")
    ax.set_ylabel("count")
    ax.set_title("World unit and city totals (save timeline)")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=9)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "world_unit_city_save_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
