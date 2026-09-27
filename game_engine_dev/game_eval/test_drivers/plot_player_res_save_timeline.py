#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    amts = []
    seat = None
    sum_v = None
    res = None
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
                        sum_v = int(part.split("=", 1)[1])
                    elif part.startswith("res="):
                        res = part.split("=", 1)[1]
                continue
            if line.startswith("turn"):
                continue
            a, b = line.split()
            turns.append(int(a))
            amts.append(int(b))
    return seat, sum_v, res, turns, amts

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 4:
        print("usage: plot_player_res_save_timeline.py <data_dir> <eval_dir> <resource>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    res_name = sys.argv[3]
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    fig, ax = plt.subplots(figsize=(10, 6))
    for path in paths:
        seat, sum_v, res, turns, amts = load_curve(path)
        label = path.stem
        if seat is not None and sum_v is not None:
            label = f"seat {seat} (sum={sum_v})"
        ax.plot(turns, amts, linewidth=1.6, label=label)
    ax.set_xlabel("turn")
    ax.set_ylabel(res_name)
    ax.set_title(f"Player {res_name} ledger (save timeline)")
    ax.grid(True, alpha=0.3)
    ax.legend(fontsize=8)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    safe = "".join(c if c.isalnum() or c in "-_" else "_" for c in res_name.lower())
    out = eval_dir / f"player_res_save_timeline_{safe}.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
