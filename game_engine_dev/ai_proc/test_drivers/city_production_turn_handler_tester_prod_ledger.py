#!/usr/bin/env python3

import argparse
import os
import sys


# city:owner:turn:pop:san:food:prod:com:cult:sci:rel:name:start:finish
IX_CITY = 0
IX_PROD = 6
IX_NAME = 11
IX_FINISH = 13
N_FIELDS = 14


def parse_name_cost(path, cost_idx):
    costs = {}
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = [p.strip() for p in line.split(":")]
            if len(parts) <= cost_idx:
                continue
            name = parts[0]
            if not name:
                continue
            try:
                costs[name] = int(parts[cost_idx])
            except ValueError:
                continue
    return costs


def load_cost_tables(game_dir):
    return {
        "building": parse_name_cost(os.path.join(game_dir, "game_config.buildings"), 1),
        "wonder": parse_name_cost(os.path.join(game_dir, "game_config.wonders"), 1),
        "small_wonder": parse_name_cost(os.path.join(game_dir, "game_config.small_wonders"), 1),
        "unit": parse_name_cost(os.path.join(game_dir, "game_config.units"), 3),
    }


def lookup_cost(name, costs):
    for key in ("building", "wonder", "small_wonder", "unit"):
        if name in costs[key]:
            return costs[key][name]
    return None


def parse_cities_trace(path, costs):
    by_city = {}
    unknown = set()
    bld_names = costs["building"]
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
                prod = int(parts[IX_PROD])
                finish = int(parts[IX_FINISH])
            except ValueError:
                continue
            name = parts[IX_NAME]
            row = by_city.get(city)
            if row is None:
                row = {"yield": 0, "cost": 0, "bld_n": 0}
                by_city[city] = row
            row["yield"] += prod
            if finish != 1 or not name:
                continue
            if name in bld_names:
                row["bld_n"] += 1
            c = lookup_cost(name, costs)
            if c is None:
                unknown.add(name)
            else:
                row["cost"] += c
    return by_city, unknown


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default_game = os.path.normpath(os.path.join(here, "..", ".."))
    ap = argparse.ArgumentParser(
        description="Per-city production ledger: deficit and building count, one city per line"
    )
    ap.add_argument(
        "--data-dir",
        default="/home/w/Projects/simple-map-gen/city-production-turn-handler-test",
    )
    ap.add_argument("--trace", default=None, help="cities.trace path (default: data-dir/cities.trace)")
    ap.add_argument("--game-dir", default=default_game)
    args = ap.parse_args()
    trace = args.trace or os.path.join(args.data_dir, "cities.trace")
    if not os.path.isfile(trace):
        print("missing trace", trace, file=sys.stderr)
        return 1
    costs = load_cost_tables(args.game_dir)
    by_city, unknown_all = parse_cities_trace(trace, costs)
    if unknown_all:
        print("unknown finished items:", sorted(unknown_all), file=sys.stderr)
    rows = []
    for city in by_city:
        row = by_city[city]
        balance = row["yield"] - row["cost"]
        deficit = row["cost"] - row["yield"]
        rows.append((balance, deficit, row["bld_n"]))
    rows.sort(key=lambda r: r[0])
    for _, deficit, bld_n in rows:
        print("%d %d" % (deficit, bld_n))
    return 0


if __name__ == "__main__":
    sys.exit(main())
