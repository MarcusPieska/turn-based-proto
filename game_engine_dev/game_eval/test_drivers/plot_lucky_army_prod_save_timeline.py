#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    vals = []
    seat = None
    total = None
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                for part in line[1:].split():
                    if part.startswith("seat="):
                        seat = int(part.split("=", 1)[1])
                    elif part.startswith("sum="):
                        total = int(part.split("=", 1)[1])
                continue
            if line.startswith("turn"):
                continue
            a, b = line.split()
            turns.append(int(a))
            vals.append(int(b))
    return seat, total, turns, vals

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_lucky_army_prod_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    series = [load_curve(p) for p in paths]
    y_max = 0
    for _seat, _total, _turns, vals in series:
        if vals:
            y_max = max(y_max, max(vals))
    if y_max < 1:
        y_max = 1
    n = len(paths)
    fig_h = max(2.2 * n, 4.0)
    fig, axes = plt.subplots(n, 1, figsize=(11, fig_h), sharex=True)
    if n == 1:
        axes = [axes]
    for ax, path, (seat, total, turns, vals) in zip(axes, paths, series):
        xs = list(range(len(turns)))
        ax.bar(xs, vals, width=0.8, color="#3d6ea5")
        title = path.stem
        if seat is not None:
            title = f"seat {seat}"
            if total is not None:
                title = f"{title} (sum={total})"
        ax.set_ylabel("new army idx")
        ax.set_title(title, loc="left", fontsize=10)
        ax.grid(True, axis="y", alpha=0.3)
        ax.set_ylim(0, y_max * 1.05)
        if turns:
            ax.set_xlim(-0.6, len(turns) - 0.4)
            tick_pos = list(range(0, len(turns), 10))
            ax.set_xticks(tick_pos)
            ax.set_xticklabels([str(turns[i]) for i in tick_pos])
    axes[-1].set_xlabel("save turn (interval end)")
    fig.suptitle("Lucky seat land-army unit index births between saves", fontsize=12)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "lucky_army_prod_save_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
