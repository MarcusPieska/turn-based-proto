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

from p1_tester_driver import read_seed_file

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

TESTERS = [
    "p1_gen_cont_outlines",
    "p1_adj_outline_fill",
    "p1_gen_noise_perlin",
    "p1_gen_land_depth",
    "p1_gen_shaped_outline",
    "p1_gen_river_pts",
    "p1_gen_river_sectors",
    "p1_gen_coastal_mtn_limits",
    "p1_gen_river_network",
    "p1_gen_river_lines",
    "p1_adj_coastal_mtn_rivers",
    "p1_adj_river_lakes",
    "p1_adj_river_inlets",
    "p1_gen_watershed_mountains",
    "p1_gen_watershed_mountain_line_sets",
    "p1_gen_distance_to_river",
    "p1_gen_river_dist",
    "p1_gen_nearness_to_watershed_mtn",
    "p1_adj_land_altitude",
    "p1_adj_ensure_coasts",
    "p1_adj_ensure_seas",
    "p1_adj_ensure_river_valleys",
    "p1_adj_ensure_mtn_foothills",
    "p1_gen_climate",
    "p1_gen_desert_river_cull",
    "p1_make_map",
    "p1_gen_wind_pattern_adv",
    "p1_gen_loess_boost",
    "p1_adj_grassland_loess_tiles",
    "p1_gen_rain_orographic",
    "p1_gen_rich_coast_fertility",
    "p1_adj_coast_fertility",
    "p1_adj_ensure_adj_rules",
    "p1_gen_forest_overlay",
    "p1_adj_delta_swamps",
]

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def _run_one(mod):
    comp = os.path.join(ROOT, "%s_comp" % mod)
    exe = os.path.join(ROOT, "%s_tester" % mod)
    if not os.path.isfile(comp):
        return False, "missing comp: %s" % comp
    proc = subprocess.run(["bash", comp], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "compile failed"
        return False, msg
    proc = subprocess.run([exe], cwd=ROOT, capture_output=True, text=True)
    if proc.returncode != 0:
        msg = proc.stderr.strip() or proc.stdout.strip() or "tester failed"
        return False, msg
    return True, proc.stdout.strip()

def main():
    seed = read_seed_file()
    print("running %d p1 testers with seed %d (subdir output)" % (len(TESTERS), seed))
    failed = []
    for mod in TESTERS:
        print("--- %s ---" % mod)
        ok, msg = _run_one(mod)
        if ok:
            if msg:
                print(msg)
        else:
            print("FAILED: %s" % msg)
            failed.append(mod)
    if failed:
        print("failed (%d): %s" % (len(failed), ", ".join(failed)))
        return 1
    print("all testers passed")
    return 0

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    raise SystemExit(main())

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
