#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Units -
#================================================================================================================================

UNITS = (
    ("s", 1e9),
    ("ms", 1e6),
    ("us", 1e3),
    ("ns", 1.0),
)

def pick_unit(max_avg_ns: float):
    for name, div in UNITS:
        if max_avg_ns / div > 1.0:
            return name, div
    return "ns", 1.0

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_samples(path: Path):
    series = {}
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            tag, val = line.split("=", 1)
            tag = tag.strip()
            try:
                ns = float(val.strip())
            except ValueError:
                continue
            series.setdefault(tag, []).append(ns)
    return series

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) != 3:
        print("usage: plot_prof_suite.py <prof_*.txt> <out.png>")
        return 1
    src = Path(sys.argv[1])
    out = Path(sys.argv[2])
    series = load_samples(src)
    if not series:
        print(f"no samples in {src}", file=sys.stderr)
        return 1
    avgs = {t: (sum(v) / len(v)) for t, v in series.items() if v}
    unit, div = pick_unit(max(avgs.values()))
    fig, ax = plt.subplots(figsize=(10, 5))
    for tag in sorted(series.keys()):
        ys = [v / div for v in series[tag]]
        xs = list(range(1, len(ys) + 1))
        ax.plot(xs, ys, linewidth=1.0, label=tag)
    ax.set_xlabel("call index")
    ax.set_ylabel(f"time ({unit})")
    ax.set_title(src.stem)
    ax.legend(loc="upper right", fontsize=7, ncol=2)
    ax.grid(True, alpha=0.3)
    out.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out, dpi=140)
    plt.close(fig)
    print(f"wrote {out} unit={unit}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
