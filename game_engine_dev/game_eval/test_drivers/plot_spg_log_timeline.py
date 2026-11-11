#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    rows = []
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("turn"):
                continue
            a, b, c, d = line.split()
            rows.append((int(a), int(b), int(c), int(d)))
    return rows

def series_for(rows, inst, col):
    turns = []
    vals = []
    for turn, i, chunks, wb in rows:
        if i != inst:
            continue
        turns.append(turn)
        vals.append(chunks if col == "chunks" else wb)
    return turns, vals

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 2:
        print("usage: plot_spg_log_timeline.py <eval_dir>")
        return 1
    eval_dir = Path(sys.argv[1])
    curve = eval_dir / "spg_log_timeline.txt"
    if not curve.is_file():
        print(f"missing {curve}")
        return 1
    rows = load_curve(curve)
    if not rows:
        print(f"empty curve {curve}")
        return 1
    fig, axes = plt.subplots(2, 2, figsize=(11, 8), sharex=True)
    specs = [
        (0, 0, 0, "chunks", "inst0 chunks"),
        (0, 1, 0, "wb", "inst0 wb"),
        (1, 0, 1, "chunks", "inst1 chunks"),
        (1, 1, 1, "wb", "inst1 wb"),
    ]
    for r, c, inst, col, title in specs:
        ax = axes[r][c]
        turns, vals = series_for(rows, inst, col)
        if turns:
            ax.plot(turns, vals, linewidth=2.2)
        ax.set_title(title)
        ax.set_ylabel(col)
        ax.grid(True, alpha=0.3)
    axes[1][0].set_xlabel("turn")
    axes[1][1].set_xlabel("turn")
    fig.suptitle("SectorPathGradients checkout")
    fig.tight_layout()
    out = eval_dir / "spg_log_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
