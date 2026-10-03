#!/usr/bin/env python3
import sys
from pathlib import Path

import matplotlib.pyplot as plt

#================================================================================================================================
#=> - Load -
#================================================================================================================================

def load_meta(data_dir: Path):
    max_turn = 0
    p = data_dir / "meta.txt"
    if not p.is_file():
        return max_turn
    for line in p.read_text().splitlines():
        if line.startswith("max_turn="):
            max_turn = int(line.split("=", 1)[1])
    return max_turn

def load_seat(path: Path):
    seat = None
    events = []
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
            t, k, a, b = line.split()
            events.append((int(t), k, int(a), int(b)))
    return seat, events

#================================================================================================================================
#=> - Tags -
#================================================================================================================================

def clip_tag(word, n=None):
    if n is None:
        return word[:7]
    s = f"{word}{n}"
    if len(s) < 8:
        return s
    s = f"{word}{n}"[:7]
    return s

def ev_tag(k, a, b):
    if k == "S":
        return clip_tag("war", a), "#c0392b"
    if k == "E":
        return clip_tag("peace", a), "#27ae60"
    if k == "L":
        return clip_tag("loss", a), "#c0392b"
    if k == "M":
        return clip_tag("mst", a), "#27ae60"
    if k == "T":
        return clip_tag("tgt", a), "#27ae60"
    if k == "A":
        return clip_tag("army", a), "#27ae60"
    if k == "F":
        return clip_tag("form"), "#e67e22"
    if k == "D":
        return clip_tag("decl", a), "#8e44ad"
    if k == "X":
        return clip_tag("assault"), "#c0392b"
    return None, None

#================================================================================================================================
#=> - Plot -
#================================================================================================================================

def main():
    if len(sys.argv) < 3:
        print("usage: plot_lucky_war_log_timeline.py <data_dir> <eval_dir>")
        return 1
    data_dir = Path(sys.argv[1])
    eval_dir = Path(sys.argv[2])
    paths = sorted(data_dir.glob("seat_*.txt"))
    if not paths:
        print(f"no seat_*.txt in {data_dir}")
        return 1
    max_turn = load_meta(data_dir)
    rows = []
    for path in paths:
        seat, events = load_seat(path)
        rows.append((seat, events))
        for t, _, _, _ in events:
            if t > max_turn:
                max_turn = t
    if max_turn < 1:
        max_turn = 1
    n = len(rows)
    band = 4.0
    fig_h = max(8.0, 1.15 * n * band)
    fig_w = max(18.0, max_turn / 40.0)
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))
    # Rails 0..15 top->bottom; cycle [0,15,1,14,...,7,8].
    rail_dy = tuple([1.35 - 0.15 * i for i in range(8)] + [-0.30 - 0.15 * i for i in range(8)])
    cycle = (0, 15, 1, 14, 2, 13, 3, 12, 4, 11, 5, 10, 6, 9, 7, 8)
    stack_dy = 0.12
    for i, (seat, events) in enumerate(rows):
        y = (n - 1 - i) * band + 1.5
        ax.hlines(y, 0, max_turn, colors="#444444", linewidths=1.0, zorder=1)
        ax.text(-max_turn * 0.01, y, f"seat {seat}", ha="right", va="center", fontsize=9)
        rail = 0
        last_t = None
        rail_i = 0
        text_y = y
        for t, k, a, b in events:
            if k == "C":
                ax.plot(t, y, "+", color="#2980b9", markersize=10, markeredgewidth=1.8, zorder=3)
                continue
            tag, col = ev_tag(k, a, b)
            if tag is None:
                continue
            if k == "S":
                ax.plot(t, y, "o", color=col, markersize=7, zorder=3)
            elif k == "E":
                ax.plot(t, y, "o", color=col, markersize=7, zorder=3)
            else:
                ax.plot(t, y, "o", color=col, markersize=3.5, zorder=3)
            if last_t != t:
                rail_i = cycle[rail % 16]
                rail += 1
                text_y = y + rail_dy[rail_i]
                last_t = t
            else:
                if rail_i < 8:
                    text_y = text_y - stack_dy
                else:
                    text_y = text_y + stack_dy
            ax.plot([t, t], [y, text_y], color="#888888", linewidth=0.7, zorder=2)
            ax.text(t + max_turn * 0.002, text_y, tag, color=col, fontsize=7, ha="left", va="center", zorder=4)
        for dy in rail_dy:
            ax.hlines(y + dy, 0, max_turn, colors="#eeeeee", linewidths=0.45, zorder=0)
    ax.set_xlim(-max_turn * 0.06, max_turn * 1.02)
    ax.set_ylim(-1.5, n * band - 0.5)
    ax.set_xlabel("turn")
    ax.set_yticks([])
    ax.set_title("Lucky war log (war/peace + C capture; mst/army/tgt/loss/form/decl/assault)")
    ax.grid(True, axis="x", alpha=0.25)
    fig.tight_layout()
    eval_dir.mkdir(parents=True, exist_ok=True)
    out = eval_dir / "lucky_war_log_timeline.png"
    fig.savefig(out, dpi=140)
    print(f"wrote {out}")
    return 0

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================
#=> - End of file -
#================================================================================================================================
