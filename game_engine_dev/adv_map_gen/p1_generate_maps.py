#!/usr/bin/env python3

#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import argparse
import os
import subprocess
import sys

sys.dont_write_bytecode = True

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

ROOT = os.path.dirname(os.path.abspath(__file__))
OUT_ROOT = "/home/w/Projects/simple-map-gen"
MAPS_ROOT = os.path.join(OUT_ROOT, "maps")
MAKE_MAP_MOD = "p1_make_map"
MAP_W_DEF = 1000
MAP_H_DEF = 1000
SEED_START_DEF = 0
SEED_END_DEF = 100
EXPORT_KINDS = ("terrain", "climate", "rivers")

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def map_export_path(seed, kind):
    return os.path.join(MAPS_ROOT, "seed-%04d-%s.ppm" % (seed, kind))

def map_exports_ok(seed):
    return all(os.path.isfile(map_export_path(seed, k)) for k in EXPORT_KINDS)

def compile_make_map():
    comp = os.path.join(ROOT, "%s_comp" % MAKE_MAP_MOD)
    if not os.path.isfile(comp):
        return False, "missing comp: %s" % comp
    proc = subprocess.run(["bash", comp], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "compile failed"
        return False, msg
    return True, ""

def run_make_map(seed, map_w, map_h):
    exe = os.path.join(ROOT, "%s_tester" % MAKE_MAP_MOD)
    if not os.path.isfile(exe):
        return False, "missing executable: %s" % exe
    proc = subprocess.run(
        [exe, str(seed), str(map_w), str(map_h)],
        cwd=ROOT,
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "make_map failed"
        return False, msg
    if not map_exports_ok(seed):
        missing = [map_export_path(seed, k) for k in EXPORT_KINDS if not os.path.isfile(map_export_path(seed, k))]
        return False, "missing export(s): %s" % ", ".join(missing)
    return True, proc.stdout.strip()

def parse_args():
    p = argparse.ArgumentParser(description="Generate P1 map exports for a seed interval.")
    p.add_argument("--start", type=int, default=SEED_START_DEF, help="first seed (default %d)" % SEED_START_DEF)
    p.add_argument("--end", type=int, default=SEED_END_DEF, help="last seed inclusive (default %d)" % SEED_END_DEF)
    p.add_argument("--width", type=int, default=MAP_W_DEF, help="map width (default %d)" % MAP_W_DEF)
    p.add_argument("--height", type=int, default=MAP_H_DEF, help="map height (default %d)" % MAP_H_DEF)
    p.add_argument("--skip-existing", action="store_true", help="skip seeds with all three exports present")
    return p.parse_args()

def main():
    args = parse_args()
    if args.start < 0 or args.end < args.start:
        print("invalid seed interval: %d..%d" % (args.start, args.end))
        return 1
    if args.width <= 0 or args.height <= 0:
        print("invalid map size: %d x %d" % (args.width, args.height))
        return 1
    os.makedirs(MAPS_ROOT, exist_ok=True)
    print("compiling %s ..." % MAKE_MAP_MOD)
    ok, msg = compile_make_map()
    if not ok:
        print("FAILED compile: %s" % msg)
        return 1
    seeds = list(range(args.start, args.end + 1))
    print("generating maps for seeds %d..%d -> %s" % (args.start, args.end, MAPS_ROOT))
    failed = []
    skipped = 0
    for seed in seeds:
        if args.skip_existing and map_exports_ok(seed):
            skipped += 1
            continue
        print("--- seed %d ---" % seed)
        ok, msg = run_make_map(seed, args.width, args.height)
        if ok:
            if msg:
                for line in msg.splitlines():
                    if line.startswith("saved:") and "/maps/" in line:
                        print(line)
        else:
            print("FAILED seed %d: %s" % (seed, msg))
            failed.append(seed)
    done = len(seeds) - len(failed) - skipped
    print("done: %d generated, %d skipped, %d failed" % (done, skipped, len(failed)))
    if failed:
        print("failed seeds: %s" % ", ".join(str(s) for s in failed))
        return 1
    return 0

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
