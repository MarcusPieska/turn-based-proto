#!/usr/bin/env python3
import re
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Functions -
#================================================================================================================================

LINE_RE = re.compile(
    r"^turn=(\d+)\s+pop=(\d+)\s+sanitation=(-?\d+)\s+sanitation_boost=(\d+)\s*$"
)

def parse_log(path: Path):
    turns = []
    pops = []
    nets = []
    boosts = []
    header = ""
    with path.open() as f:
        for line in f:
            line = line.rstrip("\n")
            if line.startswith("#"):
                header = line[1:].strip()
                continue
            m = LINE_RE.match(line)
            if not m:
                continue
            turns.append(int(m.group(1)))
            pops.append(int(m.group(2)))
            nets.append(int(m.group(3)))
            boosts.append(int(m.group(4)))
    return header, turns, pops, nets, boosts


def plot_one(idx: str, header: str, turns, pops, nets, boosts, out_path: Path):
    fig, ax = plt.subplots(figsize=(12, 5))
    ax.plot(turns, pops, "o-", markersize=3, linewidth=1.2, label="population")
    ax.plot(turns, boosts, "s-", markersize=3, linewidth=1.2, label="sanitation_boost")

    y_lo = min(pops)
    y_hi = max(pops)
    y_mid = 0.5 * (y_lo + y_hi)
    y_span = max(y_hi - y_lo, 1)
    dy = 0.04 * y_span

    for t, pop, net in zip(turns, pops, nets):
        if pop <= y_mid:
            va = "bottom"
            y_txt = pop + dy
        else:
            va = "top"
            y_txt = pop - dy
        ax.text(t, y_txt, str(net), fontsize=6, ha="center", va=va, color="0.25")

    title = f"{idx}"
    if header:
        title = f"{idx}  ({header})"
    ax.set_title(title)
    ax.set_xlabel("turn")
    ax.set_ylabel("value")
    ax.set_xlim(0, 1000)
    ax.set_ylim(0, 30)
    ax.legend(loc="best")
    ax.grid(True, alpha=0.3)
    fig.tight_layout()
    fig.savefig(out_path, dpi=140)
    plt.close(fig)


#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    root = Path("/home/w/Projects/simple-map-gen/single-city-dev-test")
    if len(sys.argv) > 1:
        root = Path(sys.argv[1]).resolve()

    logs = sorted(root.glob("[0-9][0-9][0-9].log"))
    if not logs:
        print(f"no NNN.log files in {root}", file=sys.stderr)
        return 1

    for log_path in logs:
        idx = log_path.stem
        header, turns, pops, nets, boosts = parse_log(log_path)
        if not turns:
            print(f"skip {log_path.name}: no pop samples")
            continue
        out_path = root / f"plt-{idx}.png"
        plot_one(idx, header, turns, pops, nets, boosts, out_path)
        print(f"wrote {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================