#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    alloc = []
    chkout = []
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("turn"):
                continue
            a, b, c = line.split()
            turns.append(int(a))
            alloc.append(float(b))
            chkout.append(float(c))
    return turns, alloc, chkout

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 2:
        print("usage: plot_wb_mng_log_timeline.py <eval_dir>")
        return 1
    eval_dir = Path(sys.argv[1])
    curve = eval_dir / "wb_mng_log_timeline.txt"
    if not curve.is_file():
        print(f"missing {curve}")
        return 1
    turns, alloc, chkout = load_curve(curve)
    if not turns:
        print(f"empty curve {curve}")
        return 1
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.plot(turns, alloc, linewidth=2.4, label="allocated")
    ax.plot(turns, chkout, linewidth=2.4, label="checked out")
    ax.set_xlabel("turn")
    ax.set_ylabel("MB")
    ax.set_title("WhiteboardMng memory")
    ax.grid(True, alpha=0.3)
    ax.legend()
    fig.tight_layout()
    out = eval_dir / "wb_mng_log_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
