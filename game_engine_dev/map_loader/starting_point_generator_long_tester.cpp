//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "map_loader.h"
#include "starting_point_generator.h"

#include "starting_point_generator_tester_helpers.cpp"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

typedef const char* cstr;

#define LONG_MAP_BASE "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_"
#define LONG_MAP_FIRST 0
#define LONG_MAP_LAST 99
#define LONG_PICK_FIRST 2
#define LONG_PICK_LAST 255
#define LONG_LATT_DIV 10

static void latt_for_map (u16 w, u16 h, u16 latt_div, u16* rows, u16* cols) {
    u16 r = h / latt_div;
    u16 c = w / latt_div;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static bool run_test (u16 pick_n, const MapTerrainData& map, u16 latt_rows, u16 latt_cols) {
    StartingPointGeneratorParams par = {};
    par.map = &map;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;
    par.pick_n = pick_n;
    StartingPointGenerator gen(par);
    if (!gen.generate()) {
        return false;
    }
    return validate_results(gen);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    u32 lim = 0;
    if (argc > 1) {
        char* end = nullptr;
        const unsigned long n = std::strtoul(argv[1], &end, 10);
        if (end == argv[1] || *end != '\0' || n == 0ul) {
            std::printf("*** usage: %s [test_limit]\n", argv[0]);
            return 1;
        }
        lim = static_cast<u32>(n);
    }
    const u32 map_n = static_cast<u32>(LONG_MAP_LAST - LONG_MAP_FIRST + 1);
    const u32 pick_n = static_cast<u32>(LONG_PICK_LAST - LONG_PICK_FIRST + 1);
    const u32 total = map_n * pick_n;
    const u32 run_n = (lim > 0u && lim < total) ? lim : total;
    u32 step = 0;
    u32 fail_n = 0;
    bool stop = false;
    char path[256];
    for (int frame = LONG_MAP_FIRST; frame <= LONG_MAP_LAST && !stop; ++frame) {
        std::snprintf(path, sizeof(path), "%s%03d.ppm", LONG_MAP_BASE, frame);
        MapTerrainData map;
        u16 latt_rows = 0;
        u16 latt_cols = 0;
        if (!MapLoader::load_terrain_ppm(path, map)) {
            for (u16 pick = LONG_PICK_FIRST; pick <= LONG_PICK_LAST && !stop; ++pick) {
                step += 1u;
                fail_n += 1u;
                std::printf("\r*** Testing: %u / %u", step, run_n);
                std::fflush(stdout);
                if (lim > 0u && step >= lim) {
                    stop = true;
                }
            }
            continue;
        }
        latt_for_map(map.width(), map.height(), LONG_LATT_DIV, &latt_rows, &latt_cols);
        for (u16 pick = LONG_PICK_FIRST; pick <= LONG_PICK_LAST && !stop; ++pick) {
            step += 1u;
            if (!run_test(pick, map, latt_rows, latt_cols)) {
                fail_n += 1u;
                std::printf("\n*** FAILED map %s pick_n %u\n", path, pick);
            }
            std::printf("\r*** Testing: %u / %u", step, run_n);
            std::fflush(stdout);
            if (lim > 0u && step >= lim) {
                stop = true;
            }
        }
    }
    std::printf("\n");
    if (lim > 0u) {
        std::printf("*** Long test: %u / %u cases (limit %u)\n", step, total, lim);
    } else {
        std::printf("*** Long test: %u cases\n", total);
    }
    std::printf("*** Failures: %u\n", fail_n);
    return fail_n > 0u ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
