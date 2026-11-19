#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_curve(path: Path):
    turns = []
    total = []
    tot_arty = []
    field = []
    field_arty = []
    seat = None
    with path.open() as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                for part in line[1:].split():
                    if part.startswith("seat="):
                        seat = int(part.split("=", 1)[1])
                continue
            if line.startswith("turn"):
                continue
            a, b, c, d, e = line.split()
            turns.append(int(a))
            total.append(int(b))
            tot_arty.append(int(c))
            field.append(int(d))
            field_arty.append(int(e))
    return seat, turns, total, tot_arty, field, field_arty

def plot_stack(series, main_key, arty_key, y_max, title, out_path):
    n = len(series)
    fig_h = max(2.2 * n, 4.0)
    fig, axes = plt.subplots(n, 1, figsize=(11, fig_h), sharex=True)
    if n == 1:
        axes = [axes]
    for ax, (seat, turns, main, arty) in zip(axes, series):
        xs = list(range(len(turns)))
        ax.bar(xs, main, width=0.82, color="#3d6ea5", label=main_key)
        ax.bar(xs, arty, width=0.48, color="#8ce99a", label=arty_key)
        name = f"seat {seat}" if seat is not None else "seat ?"
        ax.set_ylabel("units")
        ax.set_title(name, loc="left", fontsize=10)
        ax.grid(True, axis="y", alpha=0.3)
        ax.set_ylim(0, y_max * 1.05)
        if turns:
            ax.set_xlim(-0.6, len(turns) - 0.4)
            tick_pos = list(range(0, len(turns), 10))
            ax.set_xticks(tick_pos)
            ax.set_xticklabels([str(turns[i]) for i in tick_pos])
        ax.legend(loc="upper right", fontsize=7, frameon=False)
    axes[-1].set_xlabel("save turn")
    fig.suptitle(title, fontsize=12)
    fig.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(out_path, dpi=140)
    plt.close(fig)
    print(f"wrote {out_path}")

#================================================================================================================================
#=> - Main -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_lucky_army_field_save_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("curve_*.txt"))
    if not paths:
        print(f"no curve_*.txt in {data_dir}")
        return 1
    loaded = [load_curve(p) for p in paths]
    empire = []
    camp = []
    y_empire = 1
    y_camp = 1
    for seat, turns, total, tot_arty, field, field_arty in loaded:
        empire.append((seat, turns, total, tot_arty))
        camp.append((seat, turns, field, field_arty))
        if total:
            y_empire = max(y_empire, max(total))
        if field:
            y_camp = max(y_camp, max(field))
    plot_stack(
        empire,
        "land army",
        "artillery",
        y_empire,
        "Lucky seat empire land-army size vs artillery",
        eval_dir / "lucky_army_field_save_timeline_1_empire.png",
    )
    plot_stack(
        camp,
        "in army/campaign",
        "artillery",
        y_camp,
        "Lucky seat army/campaign size vs artillery",
        eval_dir / "lucky_army_field_save_timeline_2_camp.png",
    )
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
