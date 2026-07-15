#!/usr/bin/env python3

#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os
import subprocess
import sys

sys.dont_write_bytecode = True

ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, ROOT)

from p1_tester_driver import run_tester

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

TESTER_MOD = "p1_make_map"
OUT_IMAGE = "terrain.ppm"
COMP_SCRIPT = "p1_make_map_comp"

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def _parse_seed_range(argv):
    if len(argv) != 3:
        return None, "usage: %s <seed_lo> <seed_hi>" % argv[0]
    try:
        lo = int(argv[1])
        hi = int(argv[2])
    except ValueError:
        return None, "seed_lo and seed_hi must be unsigned integers"
    if lo < 0 or hi < 0:
        return None, "seed_lo and seed_hi must be unsigned integers"
    if lo > hi:
        return None, "seed_lo must be <= seed_hi"
    return (lo, hi), None

def _compile_make_map():
    comp = os.path.join(ROOT, COMP_SCRIPT)
    if not os.path.isfile(comp):
        return False, "missing comp: %s" % comp
    proc = subprocess.run(["bash", comp], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "compile failed"
        return False, msg
    return True, ""

def main():
    rng, err = _parse_seed_range(sys.argv)
    if err is not None:
        print(err)
        return 1
    lo, hi = rng
    ok, msg = _compile_make_map()
    if not ok:
        print("FAILED compile: %s" % msg)
        return 1
    failed = []
    ok_n = 0
    total = hi - lo + 1
    for seed in range(lo, hi + 1):
        ok, msg = run_tester(TESTER_MOD, OUT_IMAGE, seed=seed, batch=True)
        if ok:
            ok_n += 1
            print("seed %u: ok" % seed, flush=True)
        else:
            print("seed %u: FAILED: %s" % (seed, msg), flush=True)
            failed.append(seed)
    print("done: %u ok, %u failed (of %u)" % (ok_n, len(failed), total))
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
