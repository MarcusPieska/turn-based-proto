#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.transforms import offset_copy

#================================================================================================================================
#=> - Load -
#================================================================================================================================

G_OFF_DOTS = 3

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

def plot_dir(data_dir: Path, out_path: Path, title: str):
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    curves = []
    t0 = None
    t1 = None
    for path in paths:
        seat, land, turns, avgs, maxes = load_curve(path)
        if not turns:
            continue
        label = path.stem
        if seat is not None and land is not None:
            label = f"seat {seat} (land={land})"
        curves.append((label, turns, avgs, maxes))
        lo = turns[0]
        hi = turns[-1]
        t0 = lo if t0 is None else min(t0, lo)
        t1 = hi if t1 is None else max(t1, hi)
    if not curves:
        print(f"empty curves in {data_dir}")
        return 1
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.set_xlim(t0, t1)
    ax.set_ylim(0, 7)
    mid = len(curves) // 2
    for i, (label, turns, avgs, maxes) in enumerate(curves):
        dy = (i - mid) * G_OFF_DOTS
        tr = offset_copy(ax.transData, fig=fig, x=0, y=dy, units="dots")
        line, = ax.plot(turns, avgs, linewidth=2.4, label=label, transform=tr, clip_on=False)
        ax.plot(turns, maxes, linewidth=2.4, color=line.get_color(), linestyle="--", transform=tr, clip_on=False)
    ax.set_xlabel("turn")
    ax.set_ylabel("level")
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=140)
    print(f"wrote {out_path}")
    return 0

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_player_land_level_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    rc = 0
    rc = plot_dir(
        data_dir / "field",
        eval_dir / "player_land_level_save_timeline_field.png",
        "Player land unit level (field sample): avg (solid) vs UNIT_TRAINING_MAX (dashed)",
    ) or rc
    rc = plot_dir(
        data_dir / "lucky",
        eval_dir / "player_land_level_save_timeline_lucky.png",
        "Player land unit level (lucky seats): avg (solid) vs UNIT_TRAINING_MAX (dashed)",
    ) or rc
    return rc

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
