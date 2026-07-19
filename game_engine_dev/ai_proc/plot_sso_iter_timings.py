#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
# - Functions -
#================================================================================================================================

def load_rows(path: Path):
    rows = []
    with path.open("r", encoding="utf-8") as fp:
        for line in fp:
            parts = line.strip().split()
            if len(parts) < 2:
                continue
            name = parts[0]
            vals = [float(x) for x in parts[1:]]
            rows.append((name, vals))
    return rows


def summarize(name: str, vals: list[float]) -> None:
    n = len(vals)
    if n == 0:
        print(f"{name}: no samples")
        return
    total = sum(vals)
    avg = total / n
    early_n = max(1, n // 10)
    late_n = max(1, n // 10)
    early = sum(vals[:early_n]) / early_n
    late = sum(vals[-late_n:]) / late_n
    ratio = late / early if early > 0 else float("inf")
    print(
        f"{name}: n={n} total={total:.2f} us avg={avg:.2f} us "
        f"early10%={early:.2f} us late10%={late:.2f} us late/early={ratio:.3f} "
        f"last={vals[-1]:.2f} us"
    )


def main() -> int:
    ap = argparse.ArgumentParser(description="Plot SenseSettlingPtsOpt iter timings over time")
    ap.add_argument(
        "timings",
        nargs="?",
        default="/home/w/Projects/simple-map-gen/ai-settle-pts/iter-sso-m200-n100/sso_iter_timings.txt",
        help="sso_iter_timings.txt path",
    )
    ap.add_argument("-o", "--out", default=None, help="output png path (default: next to timings)")
    args = ap.parse_args()

    path = Path(args.timings)
    if not path.is_file():
        print(f"missing timings file: {path}", file=sys.stderr)
        return 1

    rows = load_rows(path)
    if not rows:
        print(f"no timing rows in {path}", file=sys.stderr)
        return 1

    for name, vals in rows:
        summarize(name, vals)

    fig, axes = plt.subplots(len(rows), 1, figsize=(12, 3.2 * len(rows)), sharex=True)
    if len(rows) == 1:
        axes = [axes]
    for ax, (name, vals) in zip(axes, rows):
        ax.plot(range(len(vals)), vals, linewidth=0.8)
        ax.set_ylabel("us")
        ax.set_title(name)
        ax.grid(True, alpha=0.3)
    axes[-1].set_xlabel("call index (chronological)")
    fig.tight_layout()

    out = Path(args.out) if args.out else path.with_name("sso_iter_timings.png")
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

#================================================================================================================================
# - Main -
#================================================================================================================================

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
# - End of file -
#================================================================================================================================
