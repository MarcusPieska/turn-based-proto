#!/usr/bin/env python3

#================================================================================================================================#
#=> - Imports -
#================================================================================================================================#

import os

#================================================================================================================================#
#=> - Constants -
#================================================================================================================================#

ROOT = os.path.dirname(os.path.abspath(__file__))

STEPS = [
    (1, "gen", "outline", "P1_Gen_Outline", "generate", "01_outline.ppm"),
    (2, "adj", "outline_fill", "P1_Adj_OutlineFill", "adjust", "02_outline_fill.ppm"),
    (3, "gen", "noise_perlin", "P1_Gen_NoisePerlin", "generate", "03_noise_perlin.ppm"),
    (4, "gen", "land_depth", "P1_Gen_LandDepth", "generate", "04_land_depth.ppm"),
    (5, "adj", "outline_shaping", "P1_Adj_OutlineShaping", "adjust", "05_outline_shaping.ppm"),
    (6, "gen", "river_pts", "P1_Gen_RiverPts", "generate", "08_river_pts.ppm"),
    (7, "gen", "river_sectors", "P1_Gen_RiverSectors", "generate", "09_river_sectors.ppm"),
    (8, "gen", "river_network", "P1_Gen_RiverNetwork", "generate", "10_river_network.ppm"),
    (9, "gen", "river_lines", "P1_Gen_RiverLines", "generate", "11_river_lines.ppm"),
    (10, "gen", "watershed_mountains", "P1_Gen_WatershedMountains", "generate", "10_watershed_mountains.ppm"),
    (11, "gen", "distance_to_river", "P1_Gen_DistanceToRiver", "generate", "11_distance_to_river.ppm"),
    (12, "gen", "nearness_to_watershed_mtn", "P1_Gen_NearnessToWatershedMtn", "generate", "12_nearness_watershed_mtn.ppm"),
    (13, "adj", "land_altitude", "P1_Adj_LandAltitude", "adjust", "13_land_altitude.ppm"),
    (14, "adj", "ensure_coasts", "P1_Adj_EnsureCoasts", "adjust", "14_ensure_coasts.ppm"),
    (15, "adj", "ensure_seas", "P1_Adj_EnsureSeas", "adjust", "15_ensure_seas.ppm"),
    (16, "adj", "ensure_river_valleys", "P1_Adj_EnsureRiverValleys", "adjust", "16_ensure_river_valleys.ppm"),
    (17, "adj", "ensure_mtn_foothills", "P1_Adj_EnsureMtnFoothills", "adjust", "17_ensure_mtn_foothills.ppm"),
    (18, "gen", "climate", "P1_Gen_Climate", "generate", "18_climate.ppm"),
]

GEN_CONSTANTS_STEPS = {1, 3}

#================================================================================================================================#
#=> - Functions -
#================================================================================================================================#

def apply_substitutions(content, pairs):
    for old, new in pairs:
        content = content.replace(old, new)
    return content

def gen_tester_py(step_n, kind, tag, out_file):
    mod = f"p1_{kind}_{tag}"
    title = "%02d %s" % (step_n, tag.replace("_", " "))
    tpl_path = os.path.join(ROOT, "BOILERPLATE_tester.py")
    with open(tpl_path, "r", encoding="utf-8") as f:
        body = f.read()
    pairs = [
        ("[TESTER_MOD]", mod),
        ("[OUT_IMAGE]", out_file),
        ("[VIEW_TITLE]", title),
    ]
    return apply_substitutions(body, pairs)

def gen_tester(step_n, kind, tag, cls, verb, out_file):
    mod = f"p1_{kind}_{tag}"
    fn = f"test_{mod}_basic"
    if kind == "gen":
        body = f'''//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_primitives.h"
#include "p1_tester_util.h"
#include "{mod}.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 {fn} (u32 seed, u16 map_w, u16 map_h) {{
    char out_path[320];
    if (!p1_make_out_path(seed, "{out_file}", out_path, sizeof(out_path))) {{
        std::printf("failed to ensure output dir\\n");
        return -1;
    }}
    {cls} gen(seed, map_w, map_h);
    const clock_t t0 = clock();
    const bool ok = gen.{verb}();
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {{
        std::printf("{cls} failed to {verb}\\n");
        return -1;
    }}
    std::printf("{cls} {verb} time: %.6f s (%u x %u)\\n",
        sec,
        static_cast<u32>(map_w),
        static_cast<u32>(map_h));
    gen.save_output(out_path);
    std::printf("saved: %s\\n", out_path);
    return 0;
}}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {{
    u32 seed = 0;
    u16 map_w = 0;
    u16 map_h = 0;
    p1_resolve_test_args(argc, argv, &seed, &map_w, &map_h);
    return {fn}(seed, map_w, map_h);
}}

//================================================================================================================================
//=> - End -
//================================================================================================================================
'''
    else:
        body = f'''//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "{mod}.h"
#include "game_primitives.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_map_path = "maps/in_map.ppm";

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

i32 {fn} (u32 seed) {{
    char out_path[320];
    if (!p1_make_out_path(seed, "{out_file}", out_path, sizeof(out_path))) {{
        std::printf("failed to ensure output dir\\n");
        return -1;
    }}
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(g_map_path, map)) {{
        std::printf("failed to load map: %s\\n", g_map_path);
        return -1;
    }}
    const u16 w = map.width();
    const u16 h = map.height();
    u8* terrain = map.data();
    if (terrain == nullptr || w == 0 || h == 0) {{
        std::printf("invalid map data\\n");
        return -1;
    }}
    {cls} adj(seed);
    const clock_t t0 = clock();
    adj.{verb}(terrain, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!adj.is_valid()) {{
        std::printf("{cls} failed to {verb}\\n");
        return -1;
    }}
    std::printf("{cls} {verb} time: %.6f s (%u x %u)\\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!map.save_terrain_ppm(out_path)) {{
        std::printf("failed to save map: %s\\n", out_path);
        return -1;
    }}
    std::printf("saved: %s\\n", out_path);
    return 0;
}}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {{
    return {fn}(p1_resolve_seed(argc, argv));
}}

//================================================================================================================================
//=> - End -
//================================================================================================================================
'''
    return body

def gen_comp(step_n, kind, tag):
    mod = f"p1_{kind}_{tag}"
    lines = ["#!/bin/bash", "", 'INC="-I.. -I../map_loader"' if kind == "adj" else 'INC="-I.."', ""]
    objs = []
    if step_n in GEN_CONSTANTS_STEPS:
        lines += [
            "g++ -std=c++11 $INC -c generator_constants.cpp -o generator_constants.o",
            "g++ -std=c++11 $INC -c perlin_noise.cpp -o perlin_noise.o",
        ]
        objs += ["generator_constants.o", "perlin_noise.o"]
    if kind == "adj":
        lines += [
            "g++ -std=c++11 $INC -c ../map_loader/map_terrain_validate.cpp -o map_terrain_validate.o",
            "g++ -std=c++11 $INC -c ../map_loader/map_terrain_data.cpp -o map_terrain_data.o",
            "g++ -std=c++11 $INC -c ../map_loader/map_loader.cpp -o map_loader.o",
        ]
        objs += ["map_terrain_validate.o", "map_terrain_data.o", "map_loader.o"]
    lines.append(f"g++ -std=c++11 $INC -c {mod}.cpp -o {mod}.o")
    lines.append(f"g++ -std=c++11 $INC -c {mod}_tester.cpp -o {mod}_tester.o")
    lines.append("")
    objs += [f"{mod}.o", f"{mod}_tester.o"]
    lines.append(f"g++ -std=c++11 -o {mod}_tester \\")
    for i, o in enumerate(objs):
        suffix = " \\" if i < len(objs) - 1 else " -lm"
        lines.append(f"    {o}{suffix}")
    lines.append("")
    lines.append("rm " + " \\\n    ".join(objs))
    return "\n".join(lines) + "\n"

def remove_legacy_viewers():
    legacy = os.path.join(ROOT, "p1_step_viewer.py")
    if os.path.isfile(legacy):
        os.remove(legacy)
    for name in os.listdir(ROOT):
        if name.startswith("view_p1_") and name.endswith(".py"):
            os.remove(os.path.join(ROOT, name))

def main():
    with open(os.path.join(ROOT, "BOILERPLATE_tester.py"), "r", encoding="utf-8") as f:
        if "[TESTER_MOD]" not in f.read():
            raise SystemExit("BOILERPLATE_tester.py missing placeholders")
    for step_n, kind, tag, cls, verb, out_file in STEPS:
        mod = f"p1_{kind}_{tag}"
        tester_py_path = os.path.join(ROOT, f"{mod}_tester.py")
        with open(tester_py_path, "w", encoding="utf-8") as f:
            f.write(gen_tester_py(step_n, kind, tag, out_file))
        os.chmod(tester_py_path, 0o755)
    remove_legacy_viewers()
    print("retrofit done: %d python testers" % len(STEPS))

#================================================================================================================================#
#=> - Main -
#================================================================================================================================#

if __name__ == "__main__":
    main()

#================================================================================================================================#
#=> - End -
#================================================================================================================================#
