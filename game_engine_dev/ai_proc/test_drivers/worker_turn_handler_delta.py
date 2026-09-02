#!/usr/bin/env python3
"""Compare up to 5 newest worker_turn_handler_YYYYMMDD_HHMMSS.txt result files.

Prints one metric per row: name, then up to 5 run columns.
Column 1 is absolute from the oldest file in the window.
Columns 2..5 are consecutive deltas (run_i - run_{i-1}).
Time metrics: green when delta < 0 (faster), red when > 0.
Build metrics: green when delta > 0 (more), red when < 0.
"""

from __future__ import annotations

import glob
import os
import sys

RES_DIR = "/home/w/Projects/simple-map-gen"
PREFIX = "worker_turn_handler_"
SUFFIX = ".txt"
MAX_N = 5

GRN = "\033[32m"
RED = "\033[31m"
RST = "\033[0m"


def list_result_files(res_dir: str) -> list[str]:
    pat = os.path.join(res_dir, f"{PREFIX}*{SUFFIX}")
    files = [p for p in glob.glob(pat) if os.path.isfile(p)]
    files.sort(key=lambda p: os.path.basename(p))
    return files[-MAX_N:]


def parse_file(path: str) -> dict[str, float]:
    out: dict[str, float] = {}
    with open(path, "r", encoding="utf-8") as fp:
        for line in fp:
            line = line.strip()
            if not line or line.startswith("#") or "=" not in line:
                continue
            key, raw = line.split("=", 1)
            key = key.strip()
            try:
                out[key] = float(raw.strip())
            except ValueError:
                continue
    return out


def is_time_key(key: str) -> bool:
    return key == "turns" or key.startswith("time.")


def is_build_key(key: str) -> bool:
    return key.startswith("build.")


def good_delta(key: str, d: float) -> bool | None:
    if abs(d) < 1e-12:
        return None
    if is_time_key(key) and key != "turns":
        return d < 0.0
    if is_build_key(key):
        return d > 0.0
    return None


def fmt_num(v: float, is_delta: bool) -> str:
    if abs(v - round(v)) < 1e-9:
        n = int(round(v))
        if is_delta and n > 0:
            return f"+{n}"
        return str(n)
    if is_delta and v > 0:
        return f"+{v:.3f}"
    return f"{v:.3f}"


def colorize(key: str, text: str, d: float, is_delta: bool) -> str:
    if not is_delta:
        return text
    g = good_delta(key, d)
    if g is True:
        return f"{GRN}{text}{RST}"
    if g is False:
        return f"{RED}{text}{RST}"
    return text


def short_label(path: str) -> str:
    base = os.path.basename(path)
    if base.startswith(PREFIX) and base.endswith(SUFFIX):
        return base[len(PREFIX) : -len(SUFFIX)]
    return base


def main() -> int:
    res_dir = RES_DIR
    if len(sys.argv) > 1:
        res_dir = sys.argv[1]
    files = list_result_files(res_dir)
    if not files:
        print(f"no {PREFIX}*{SUFFIX} files in {res_dir}", file=sys.stderr)
        return 1
    runs = [parse_file(p) for p in files]
    keys: list[str] = []
    seen: set[str] = set()
    for run in runs:
        for k in run:
            if k not in seen:
                seen.add(k)
                keys.append(k)

    def sort_key(k: str) -> tuple[int, str]:
        if k == "turns":
            return (0, k)
        if k.startswith("time."):
            return (1, k)
        if k.startswith("build."):
            return (2, k)
        return (3, k)

    keys.sort(key=sort_key)

    labels = [short_label(p) for p in files]
    while len(labels) < MAX_N:
        labels.append("-")
    name_w = max(len("name"), max((len(k) for k in keys), default=4))
    col_w = 14
    header = f"{'name':<{name_w}}"
    for lab in labels:
        header += f"  {lab:>{col_w}}"
    print(header)
    print("-" * len(header))

    for key in keys:
        vals = [run.get(key) for run in runs]
        row = f"{key:<{name_w}}"
        prev = None
        for i in range(MAX_N):
            if i >= len(vals) or vals[i] is None:
                row += f"  {'':>{col_w}}"
                continue
            v = vals[i]
            if i == 0 or prev is None:
                plain = fmt_num(v, False)
                row += f"  {plain:>{col_w}}"
                prev = v
                continue
            d = v - prev
            plain = fmt_num(d, True)
            padded = f"{plain:>{col_w}}"
            row += f"  {colorize(key, padded, d, True)}"
            prev = v
        print(row)
    print()
    print("files (oldest -> newest):")
    for p in files:
        print(f"  {p}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
