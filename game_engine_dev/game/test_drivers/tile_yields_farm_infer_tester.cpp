//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "build_adds_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "std_add_helper.h"
#include "tile_attr_tables.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u8 G_CLIM_N = 5u;
static const u32 G_WARM_PASSES = 2u;
static const u32 G_TIME_PASSES = 20u;

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

static bool plant_farm (GameArraySimple& map, u16 x, u16 y) {
    if (!map.set_tile_add(x, y, 0u, BUILD_ADD_STD)) {
        return false;
    }
    StdAddHelper::set_farm(map.tile(x, y));
    return true;
}

static bool clear_add (GameArraySimple& map, u16 x, u16 y) {
    return map.set_tile_add(x, y, U16_KEY_NULL, BUILD_ADD_STD);
}

static u32 plant_all_farms (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != BUILD_ADD_STD || map.get_add_idx(x, y) != U16_KEY_NULL) {
                continue;
            }
            if (plant_farm(map, x, y)) {
                n++;
            }
        }
    }
    return n;
}

static void clear_all_adds (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) == BUILD_ADD_STD) {
                clear_add(map, x, y);
            }
        }
    }
}

static u64 sweep_map (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    u64 acc = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const TileYield yld = TileYields::get(x, y);
            acc += static_cast<u64>(yld.m_food);
            acc += static_cast<u64>(yld.m_production);
            acc += static_cast<u64>(yld.m_commerce);
        }
    }
    return acc;
}

static void time_gets (cstr label, const GameArraySimple& map) {
    const u64 tile_n = static_cast<u64>(map.tile_n());
    u64 warm_acc = 0;
    for (u32 p = 0; p < G_WARM_PASSES; ++p) {
        warm_acc += sweep_map(map);
    }
    u64 timed_acc = 0;
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (u32 p = 0; p < G_TIME_PASSES; ++p) {
        timed_acc += sweep_map(map);
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const u64 total_ns = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    const u64 call_n = tile_n * static_cast<u64>(G_TIME_PASSES);
    const u64 ns_per_get = (call_n == 0u) ? 0u : (total_ns / call_n);
    const f64 ms_total = static_cast<f64>(total_ns) / 1.0e6;
    std::printf("-----------------------------------------------------------\n");
    std::printf("TILE YIELDS TIMING (%s)\n", label);
    std::printf(" map=%ux%u tiles=%llu warm_passes=%u time_passes=%u\n",
        static_cast<unsigned>(map.width()), static_cast<unsigned>(map.height()),
        static_cast<unsigned long long>(tile_n),
        static_cast<unsigned>(G_WARM_PASSES),
        static_cast<unsigned>(G_TIME_PASSES));
    std::printf(" calls=%llu total_ns=%llu total_ms=%.3f\n",
        static_cast<unsigned long long>(call_n),
        static_cast<unsigned long long>(total_ns),
        ms_total);
    std::printf(" ns_per_get=%llu checksum=%llu warm_checksum=%llu\n",
        static_cast<unsigned long long>(ns_per_get),
        static_cast<unsigned long long>(timed_acc),
        static_cast<unsigned long long>(warm_acc));
}

static cstr attr_name (u8 kind, u8 id) {
    static const struct {
        cstr m_nm;
        u8 m_kind;
        u8 m_id;
    } k_rows[] = {
        {"CLIMATE_NONE", TileAttrTables::k_kind_clim, CLIMATE_NONE},
        {"CLIMATE_DESERT", TileAttrTables::k_kind_clim, CLIMATE_DESERT},
        {"CLIMATE_PLAINS", TileAttrTables::k_kind_clim, CLIMATE_PLAINS},
        {"CLIMATE_GRASSLAND", TileAttrTables::k_kind_clim, CLIMATE_GRASSLAND},
        {"CLIMATE_BLACK_SOIL", TileAttrTables::k_kind_clim, CLIMATE_BLACK_SOIL},
        {"OV_RIVERS", TileAttrTables::k_kind_riv, 0u},
        {"OV_NONE", TileAttrTables::k_kind_ov, OV_NONE[0]},
        {"OV_FORESTS", TileAttrTables::k_kind_ov, OV_FOREST[0]},
        {"OV_SWAMPS", TileAttrTables::k_kind_ov, OV_SWAMP[0]},
        {"OV_JUNGLES", TileAttrTables::k_kind_ov, OV_JUNGLE[0]},
        {"OV_GLACIER", TileAttrTables::k_kind_ov, OV_GLACIER[0]},
    };
    const u32 n = static_cast<u32>(sizeof(k_rows) / sizeof(k_rows[0]));
    for (u32 i = 0; i < n; ++i) {
        if (k_rows[i].m_kind == kind && k_rows[i].m_id == id) {
            return k_rows[i].m_nm;
        }
    }
    return "?";
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    note_result(build_paths(), "build map paths");
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING FARM YIELD INFER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    RuntimeStatics& st = loader.statics();
    note_result(TileYields::setup(st), "setup tile yields");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING FARM YIELD INFER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    TileYields::bind_map(&map);
    const u16 w = map.width();
    const u16 h = map.height();

    i16 clim_food[G_CLIM_N];
    u32 clim_n[G_CLIM_N];
    bool clim_ok[G_CLIM_N];
    for (u8 c = 0; c < G_CLIM_N; ++c) {
        clim_food[c] = 0;
        clim_n[c] = 0;
        clim_ok[c] = true;
    }
    i16 riv_food = 0;
    i16 riv_comm = 0;
    u32 riv_n = 0;
    bool riv_ok = true;
    bool riv_set = false;
    u32 boost_n = 0;
    u32 mismatch_n = 0;
    u32 sample_n = 0;

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != BUILD_ADD_STD || map.get_add_idx(x, y) != U16_KEY_NULL) {
                continue;
            }
            if (map.get_river(x, y) != 0u) {
                continue;
            }
            const TileYield base = TileYields::get(x, y);
            if (!plant_farm(map, x, y)) {
                mismatch_n++;
                continue;
            }
            const TileYield farmed = TileYields::get(x, y);
            clear_add(map, x, y);
            if (base.m_food == 0u && farmed.m_food == 0u) {
                continue;
            }
            const i16 df = static_cast<i16>(farmed.m_food) - static_cast<i16>(base.m_food);
            const i16 dp = static_cast<i16>(farmed.m_production) - static_cast<i16>(base.m_production);
            const i16 dc = static_cast<i16>(farmed.m_commerce) - static_cast<i16>(base.m_commerce);
            if (dp != 0 || dc != 0) {
                mismatch_n++;
            }
            const u8 clim = map.get_climate(x, y);
            if (clim >= G_CLIM_N) {
                continue;
            }
            if (df != 0) {
                boost_n++;
            }
            if (clim_n[clim] == 0u) {
                clim_food[clim] = df;
            } else if (clim_food[clim] != df) {
                clim_ok[clim] = false;
            }
            clim_n[clim]++;
            if (sample_n < 4u && df != 0) {
                std::printf("sample dry (%u,%u) clim=%s food %+d\n",
                    static_cast<unsigned>(x), static_cast<unsigned>(y),
                    attr_name(TileAttrTables::k_kind_clim, clim), static_cast<int>(df));
                sample_n++;
            }
        }
    }

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != BUILD_ADD_STD || map.get_add_idx(x, y) != U16_KEY_NULL) {
                continue;
            }
            if (map.get_river(x, y) == 0u) {
                continue;
            }
            const TileYield base = TileYields::get(x, y);
            if (!plant_farm(map, x, y)) {
                mismatch_n++;
                continue;
            }
            const TileYield farmed = TileYields::get(x, y);
            clear_add(map, x, y);
            if (base.m_food == 0u && farmed.m_food == 0u && base.m_commerce == 0u && farmed.m_commerce == 0u) {
                continue;
            }
            const i16 df = static_cast<i16>(farmed.m_food) - static_cast<i16>(base.m_food);
            const i16 dp = static_cast<i16>(farmed.m_production) - static_cast<i16>(base.m_production);
            const i16 dc = static_cast<i16>(farmed.m_commerce) - static_cast<i16>(base.m_commerce);
            if (dp != 0) {
                mismatch_n++;
            }
            const u8 clim = map.get_climate(x, y);
            const i16 expect_clim = (clim < G_CLIM_N) ? clim_food[clim] : 0;
            const i16 rf = (base.m_food == 0u && farmed.m_food == 0u) ? 0 : static_cast<i16>(df - expect_clim);
            if (df != 0 || dc != 0) {
                boost_n++;
            }
            if (base.m_food > 0u || farmed.m_food > 0u) {
                if (!riv_set) {
                    riv_food = rf;
                    riv_comm = dc;
                    riv_set = true;
                } else if (riv_food != rf || riv_comm != dc) {
                    riv_ok = false;
                }
                riv_n++;
            } else if (dc != 0) {
                if (!riv_set) {
                    riv_comm = dc;
                    riv_set = true;
                } else if (riv_comm != dc) {
                    riv_ok = false;
                }
                riv_n++;
            }
            if (sample_n < 8u && (df != 0 || dc != 0)) {
                std::printf("sample riv (%u,%u) clim=%s food %+d comm %+d (riv food %+d)\n",
                    static_cast<unsigned>(x), static_cast<unsigned>(y),
                    attr_name(TileAttrTables::k_kind_clim, clim),
                    static_cast<int>(df), static_cast<int>(dc), static_cast<int>(rf));
                sample_n++;
            }
        }
    }

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != BUILD_ADD_STD || map.get_add_idx(x, y) != U16_KEY_NULL) {
                continue;
            }
            const TileYield base = TileYields::get(x, y);
            plant_farm(map, x, y);
            const TileYield farmed = TileYields::get(x, y);
            clear_add(map, x, y);
            if (base.m_food == 0u && farmed.m_food == 0u) {
                continue;
            }
            const i16 df = static_cast<i16>(farmed.m_food) - static_cast<i16>(base.m_food);
            const i16 dc = static_cast<i16>(farmed.m_commerce) - static_cast<i16>(base.m_commerce);
            const u8 clim = map.get_climate(x, y);
            i16 pred_f = (clim < G_CLIM_N) ? clim_food[clim] : 0;
            i16 pred_c = 0;
            if (map.get_river(x, y) != 0u) {
                pred_f = static_cast<i16>(pred_f + riv_food);
                pred_c = riv_comm;
            }
            if (df != pred_f || dc != pred_c) {
                mismatch_n++;
            }
        }
    }

    std::printf("-----------------------------------------------------------\n");
    std::printf("FARM YIELD INFER (food-first, seed %u, %ux%u)\n",
        static_cast<unsigned>(G_SEED), static_cast<unsigned>(w), static_cast<unsigned>(h));
    std::printf(" tiles_with_farm_boost=%u verify_mismatches=%u\n",
        static_cast<unsigned>(boost_n), static_cast<unsigned>(mismatch_n));
    std::printf(" inferred climate food boosts (dry tiles only):\n");
    for (u8 c = 0; c < G_CLIM_N; ++c) {
        if (clim_n[c] == 0u) {
            continue;
        }
        std::printf("  %-20s FOOD %+d  (n=%u%s)\n",
            attr_name(TileAttrTables::k_kind_clim, c),
            static_cast<int>(clim_food[c]),
            static_cast<unsigned>(clim_n[c]),
            clim_ok[c] ? "" : " INCONSISTENT");
        note_result(clim_ok[c], "climate boost consistent");
    }
    if (riv_n > 0u) {
        std::printf(" inferred river boosts:\n");
        std::printf("  %-20s FOOD %+d  COMMERCE %+d  (n=%u%s)\n",
            attr_name(TileAttrTables::k_kind_riv, 0u),
            static_cast<int>(riv_food),
            static_cast<int>(riv_comm),
            static_cast<unsigned>(riv_n),
            riv_ok ? "" : " INCONSISTENT");
        note_result(riv_ok, "river boost consistent");
    }
    note_result(boost_n > 0u, "saw farm boosts");
    note_result(mismatch_n == 0u, "all tiles match inferred rules");
    note_result(clim_food[CLIMATE_DESERT] == 1, "desert food +1");
    note_result(clim_food[CLIMATE_PLAINS] == 1, "plains food +1");
    note_result(clim_food[CLIMATE_GRASSLAND] == 1, "grassland food +1");
    note_result(clim_food[CLIMATE_BLACK_SOIL] == 2, "black soil food +2");
    note_result(riv_food == 1 && riv_comm == 1, "river food+1 commerce+1");
    std::printf("-----------------------------------------------------------\n");

    clear_all_adds(map);
    time_gets("without farms", map);
    const u32 farm_n = plant_all_farms(map);
    note_result(farm_n > 0u, "planted farms for timing");
    time_gets("with farms on all free STD tiles", map);
    clear_all_adds(map);
    std::printf("-----------------------------------------------------------\n");
    std::printf(" farms_planted_for_timing=%u\n", static_cast<unsigned>(farm_n));
    std::printf("-----------------------------------------------------------\n");

    TileYields::bind_map(nullptr);
    std::printf("=======================================================\n");
    std::printf(" TESTING FARM YIELD INFER: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
