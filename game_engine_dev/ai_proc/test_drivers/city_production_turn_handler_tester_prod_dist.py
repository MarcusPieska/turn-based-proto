#!/usr/bin/env python3

import argparse
import os
import sys

import matplotlib.pyplot as plt


# city:owner:turn:pop:san:food:prod:com:cult:sci:rel:name:start:finish
IX_CITY = 0
IX_TURN = 2
IX_PROD = 6
N_FIELDS = 14


def parse_cities_trace(path):
    by_city = {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split(":")
            if len(parts) < N_FIELDS:
                continue
            try:
                city = int(parts[IX_CITY])
                turn = int(parts[IX_TURN])
                prod = int(parts[IX_PROD])
            except ValueError:
                continue
            row = by_city.get(city)
            if row is None:
                by_city[city] = {"found_turn": turn, "total_prod": prod}
            else:
                if turn < row["found_turn"]:
                    row["found_turn"] = turn
                row["total_prod"] += prod
    return by_city


def main():
    ap = argparse.ArgumentParser(description="City production scatter: founding turn vs total prod yield")
    ap.add_argument(
        "--data-dir",
        default="/home/w/Projects/simple-map-gen/city-production-turn-handler-test",
    )
    ap.add_argument("--trace", default=None, help="cities.trace path (default: data-dir/cities.trace)")
    ap.add_argument("--out", default=None, help="PNG output path (default: data-dir/prod_dist.png)")
    args = ap.parse_args()
    trace = args.trace or os.path.join(args.data_dir, "cities.trace")
    if not os.path.isfile(trace):
        print("missing trace", trace, file=sys.stderr)
        return 1
    by_city = parse_cities_trace(trace)
    if not by_city:
        print("no city data in", trace, file=sys.stderr)
        return 1
    xs = []
    ys = []
    for city in sorted(by_city):
        xs.append(by_city[city]["found_turn"])
        ys.append(by_city[city]["total_prod"])
    fig, ax = plt.subplots(figsize=(10, 6))
    ax.scatter(xs, ys, alpha=0.55, s=18, edgecolors="none")
    ax.set_xlabel("founding turn")
    ax.set_ylabel("total production yield")
    ax.set_title("City production vs founding turn")
    ax.grid(True, alpha=0.3)
    out = args.out or os.path.join(args.data_dir, "prod_dist.png")
    fig.tight_layout()
    fig.savefig(out, dpi=120)
    print("wrote", out, "n=", len(by_city))
    return 0


if __name__ == "__main__":
    sys.exit(main())
