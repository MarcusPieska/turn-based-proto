#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    units = []
    seat = None
    count = None
    unit_typ = None
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
                    elif part.startswith("unit_typ="):
                        unit_typ = int(part.split("=", 1)[1])
                continue
            if line.startswith("turn"):
                continue
            a, b = line.split()
            turns.append(int(a))
            units.append(int(b))
    return seat, count, unit_typ, turns, units

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_player_sel_unit_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    unit_typ = None
    fig, ax = plt.subplots(figsize=(10, 6))
    for path in paths:
        seat, count, typ, turns, units = load_curve(path)
        if unit_typ is None and typ is not None:
            unit_typ = typ
        label = path.stem
        if seat is not None and count is not None:
            label = f"seat {seat} (n={count})"
        ax.plot(turns, units, linewidth=1.6, label=label)
    ax.set_xlabel("turn")
    ax.set_ylabel("units")
    title = "Player selected-unit count (save timeline)"
    if unit_typ is not None:
        title = f"Player selected-unit count typ={unit_typ} (save timeline)"
    ax.set_title(title)
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "player_sel_unit_save_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
