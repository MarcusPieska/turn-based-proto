#!/usr/bin/env python3
import re
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

SERIES = {
    "lost": {
        "glob": "save_*_lost.txt",
        "title": "Land combat units off-city and not in campaign",
        "ylabel": "lost field land-combat units",
        "out": "misc_unit_state_lost_field.png",
    },
    "in_campaign": {
        "glob": "save_*_in_campaign.txt",
        "title": "Land combat units in campaign with seat m_at_war==0",
        "ylabel": "campaign units without war flag",
        "out": "misc_unit_state_camp_no_war.png",
    },
}

TURN_RE = re.compile(r"save_(\d+)_(?:lost|in_campaign)\.txt$")

def load_lucky(path: Path):
    lucky = {}
    if not path.is_file():
        return lucky
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("seat"):
                continue
            a, b = line.split()
            lucky[int(a)] = int(b) != 0
    return lucky

def load_series(data_dir: Path, pattern: str):
    # seat -> list of (turn, n) sorted by turn
    by_seat = defaultdict(list)
    turns = []
    for path in sorted(data_dir.glob(pattern)):
        m = TURN_RE.search(path.name)
        if not m:
            continue
        turn = int(m.group(1))
        turns.append(turn)
        with path.open() as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith("#") or line.startswith("seat"):
                    continue
                seat, n = line.split()
                by_seat[int(seat)].append((turn, int(n)))
    for seat in by_seat:
        by_seat[seat].sort(key=lambda t: t[0])
    return sorted(set(turns)), by_seat

def plot_series(data_dir: Path, eval_dir: Path, lucky: dict, key: str, cfg: dict):
    turns, by_seat = load_series(data_dir, cfg["glob"])
    if not turns:
        print(f"no files for {cfg['glob']} in {data_dir}")
        return 1
    fig, ax = plt.subplots(figsize=(11, 6.5))
    world = []
    for t in turns:
        tot = 0
        for seat, series in by_seat.items():
            for st, n in series:
                if st == t:
                    tot += n
                    break
        world.append(tot)
    ax.plot(turns, world, linewidth=2.0, color="black", label="world total", zorder=3)
    shown = 0
    for seat in sorted(by_seat):
        series = by_seat[seat]
        vals = [n for _, n in series]
        if not any(v > 0 for v in vals):
            continue
        xs = [t for t, _ in series]
        is_lucky = lucky.get(seat, False)
        marker = "o" if is_lucky else "x"
        tag = "lucky" if is_lucky else "other"
        ax.plot(
            xs,
            vals,
            linewidth=1.2,
            marker=marker,
            markersize=4.0 if is_lucky else 3.5,
            label=f"seat {seat} ({tag})",
            zorder=2,
        )
        shown += 1
    ax.set_xlabel("turn")
    ax.set_ylabel(cfg["ylabel"])
    ax.set_title(cfg["title"])
    ax.grid(True, alpha=0.3)
    if shown <= 24:
        ax.legend(fontsize=7, ncol=2)
    else:
        ax.legend(fontsize=6, ncol=3)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / cfg["out"]
    fig.savefig(out, dpi=140)
    print(f"wrote {out} seats_plotted={shown}")
    return 0

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_misc_unit_state_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    lucky = load_lucky(data_dir / "lucky.txt")
    rc = 0
    for key, cfg in SERIES.items():
        rc = plot_series(data_dir, eval_dir, lucky, key, cfg) or rc
    return rc

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
