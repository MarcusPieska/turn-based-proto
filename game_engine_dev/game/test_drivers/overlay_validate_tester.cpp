//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "build_adds_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_overlay_enum.h"
#include "std_add_helper.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool find_empty (const GameArraySimple& map, u16* ox, u16* oy) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            if (map.get_res(x, y) != U16_KEY_NULL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

struct TileSnap {
    u16 m_ov;
    u16 m_idx;
};

static TileSnap snap (const GameArraySimple& map, u16 x, u16 y) {
    TileSnap s;
    s.m_ov = map.get_overlay(x, y);
    s.m_idx = map.get_add_idx(x, y);
    return s;
}

static void restore (GameArraySimple& map, u16 x, u16 y, TileSnap s) {
    map.set_overlay(x, y, s.m_ov);
    map.set_add_idx(x, y, s.m_idx);
}

static void test_catalog_and_payload (GameArraySimple& map, u16 x, u16 y) {
    const TileSnap bak = snap(map, x, y);
    if (print_level > 0) {
        std::printf("*** tile (%u,%u) ov=%u idx=%u\n",
            static_cast<unsigned>(x),
            static_cast<unsigned>(y),
            static_cast<unsigned>(bak.m_ov),
            static_cast<unsigned>(bak.m_idx));
    }
    note_result(!map.set_overlay(x, y, 999u), "overlay: reject unknown catalog index");
    note_result(map.set_overlay(x, y, static_cast<u16>(MapOverlay::Farm)), "overlay: stamp Farm");
    note_result(map.set_add_idx(x, y, StdAddHelper::m_irr_bit), "payload: Farm accepts Irrigation bit");
    note_result(map.get_add_idx(x, y) == StdAddHelper::m_irr_bit, "payload: Farm Irrigation stored");
    note_result(!map.set_add_idx(x, y, 8u), "payload: reject illegal Farm bit");
    note_result(map.set_overlay(x, y, static_cast<u16>(MapOverlay::Forest)), "overlay: Farm -> Forest clears payload");
    note_result(map.get_add_idx(x, y) == 0u, "overlay: add_idx cleared on change");
    note_result(map.set_add_idx(x, y, StdAddHelper::m_mill_bit), "payload: Forest accepts Saw Mill bit");
    note_result(!map.set_add_idx(x, y, StdAddHelper::m_irr_bit), "payload: reject Irrigation on Forest");
    note_result(map.set_overlay(x, y, static_cast<u16>(MapOverlay::Mine)), "overlay: stamp Mine");
    note_result(!map.set_add_idx(x, y, 1u), "payload: reject nonzero Mine payload");
    note_result(map.set_tile_add(x, y, 3u, BUILD_ADD_CITY), "legacy: city via set_tile_add");
    note_result(map.get_overlay(x, y) == static_cast<u16>(MapOverlay::City), "legacy: city overlay set");
    note_result(map.get_add_idx(x, y) == 3u, "legacy: city external key stored");
    restore(map, x, y, bak);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map gen data");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" OVERLAY VALIDATE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    if (print_level > 0) {
        std::printf("*** map %u x %u seed=%u print_level=%d\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            G_SEED,
            print_level);
    }
    u16 x = 0;
    u16 y = 0;
    note_result(find_empty(map, &x, &y), "find empty plains tile");
    if (total_test_fails == 0) {
        test_catalog_and_payload(map, x, y);
    }
    std::printf("=======================================================\n");
    std::printf(" OVERLAY VALIDATE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
