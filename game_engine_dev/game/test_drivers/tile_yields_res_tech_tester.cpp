//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>
#include <vector>

#include "bit_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u32 G_WARM_PASSES = 2u;
static const u32 G_TIME_PASSES = 20u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

struct ResCoord {
    u16 m_x;
    u16 m_y;
};

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static void fill_tech (BitArrayCL& tech) {
    const u32 n = tech.get_count();
    for (u32 i = 0; i < n; ++i) {
        tech.set_bit(i);
    }
}

static void collect_res (const GameArraySimple& map, std::vector<ResCoord>* out) {
    out->clear();
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_res(x, y) != U16_KEY_NULL) {
                ResCoord c;
                c.m_x = x;
                c.m_y = y;
                out->push_back(c);
            }
        }
    }
}

static void snap_yld (const std::vector<ResCoord>& coords, std::vector<TileYield>* out) {
    out->resize(coords.size());
    for (u32 i = 0; i < coords.size(); ++i) {
        (*out)[i] = TileYields::get(coords[i].m_x, coords[i].m_y);
    }
}

static void time_res_gets (cstr label, const std::vector<ResCoord>& coords) {
    const u64 tile_n = static_cast<u64>(coords.size());
    u64 warm_acc = 0;
    for (u32 p = 0; p < G_WARM_PASSES; ++p) {
        for (u32 i = 0; i < coords.size(); ++i) {
            const TileYield yld = TileYields::get(coords[i].m_x, coords[i].m_y);
            warm_acc += static_cast<u64>(yld.m_food);
            warm_acc += static_cast<u64>(yld.m_production);
            warm_acc += static_cast<u64>(yld.m_commerce);
        }
    }
    u64 timed_acc = 0;
    const auto t0 = std::chrono::high_resolution_clock::now();
    for (u32 p = 0; p < G_TIME_PASSES; ++p) {
        for (u32 i = 0; i < coords.size(); ++i) {
            const TileYield yld = TileYields::get(coords[i].m_x, coords[i].m_y);
            timed_acc += static_cast<u64>(yld.m_food);
            timed_acc += static_cast<u64>(yld.m_production);
            timed_acc += static_cast<u64>(yld.m_commerce);
        }
    }
    const auto t1 = std::chrono::high_resolution_clock::now();
    const u64 total_ns = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    const u64 call_n = tile_n * static_cast<u64>(G_TIME_PASSES);
    const u64 ns_per_get = (call_n == 0u) ? 0u : (total_ns / call_n);
    const f64 ms_total = static_cast<f64>(total_ns) / 1.0e6;
    std::printf("-----------------------------------------------------------\n");
    std::printf("TILE YIELDS TIMING (%s)\n", label);
    std::printf(" res_tiles=%llu warm_passes=%u time_passes=%u\n",
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

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    note_result(build_paths(), "build map paths");
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING RES TECH YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    RuntimeStatics& st = loader.statics();
    note_result(TileYields::setup(st), "setup tile yields");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING RES TECH YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    std::vector<ResCoord> res_coords;
    collect_res(map, &res_coords);
    note_result(res_coords.size() > 0u, "map has resource tiles");
    std::printf("resource_tiles=%llu map=%ux%u\n",
        static_cast<unsigned long long>(res_coords.size()),
        static_cast<unsigned>(map.width()),
        static_cast<unsigned>(map.height()));

    const u16 tech_n = st.tech().get_item_count();
    note_result(tech_n > 0u, "tech catalog non-empty");
    BitArrayCL tech(tech_n);
    fill_tech(tech);
    TileYieldCtx ctx = {};
    ctx.m_tech = &tech;
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&ctx);

    std::vector<TileYield> base;
    snap_yld(res_coords, &base);
    time_res_gets("all techs researched", res_coords);

    const u16 res_n = st.resource().get_item_count();
    std::vector<u8> res_hit(res_n, 0u);
    u32 hit_n = 0;
    u64 sweep_ns = 0;
    u64 sweep_calls = 0;
    std::printf("-----------------------------------------------------------\n");
    std::printf("TECH DISABLE YIELD DELTAS (resource tiles only)\n");
    for (u16 ti = 0; ti < tech_n; ++ti) {
        tech.clear_bit(ti);
        u32 chg_n = 0;
        bool ex_set = false;
        u16 ex_x = 0;
        u16 ex_y = 0;
        u16 ex_ri = U16_KEY_NULL;
        i32 ex_df = 0;
        i32 ex_dp = 0;
        i32 ex_dc = 0;
        for (u16 ri = 0; ri < res_n; ++ri) {
            res_hit[ri] = 0u;
        }
        const auto t0 = std::chrono::high_resolution_clock::now();
        for (u32 i = 0; i < res_coords.size(); ++i) {
            const u16 x = res_coords[i].m_x;
            const u16 y = res_coords[i].m_y;
            const TileYield yld = TileYields::get(x, y);
            const i32 dfi = static_cast<i32>(base[i].m_food) - static_cast<i32>(yld.m_food);
            const i32 dpi = static_cast<i32>(base[i].m_production) - static_cast<i32>(yld.m_production);
            const i32 dci = static_cast<i32>(base[i].m_commerce) - static_cast<i32>(yld.m_commerce);
            if (dfi == 0 && dpi == 0 && dci == 0) {
                continue;
            }
            chg_n++;
            const u16 ri = map.get_res(x, y);
            if (ri < res_n) {
                res_hit[ri] = 1u;
            }
            if (!ex_set) {
                ex_set = true;
                ex_x = x;
                ex_y = y;
                ex_ri = ri;
                ex_df = dfi;
                ex_dp = dpi;
                ex_dc = dci;
            }
        }
        const auto t1 = std::chrono::high_resolution_clock::now();
        sweep_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        sweep_calls += static_cast<u64>(res_coords.size());
        tech.set_bit(ti);
        if (chg_n == 0u) {
            continue;
        }
        hit_n++;
        cstr nm = st.tech().get_name(TechStaticDataKey::from_raw(ti));
        if (nm == nullptr) {
            nm = "?";
        }
        cstr ex_rnm = "?";
        if (ex_ri < res_n) {
            ex_rnm = st.resource().get_name(ResourceStaticDataKey::from_raw(ex_ri));
            if (ex_rnm == nullptr) {
                ex_rnm = "?";
            }
        }
        std::printf(" tech[%u]=%s tiles=%u example (%u,%u) %s",
            static_cast<unsigned>(ti), nm, static_cast<unsigned>(chg_n),
            static_cast<unsigned>(ex_x), static_cast<unsigned>(ex_y), ex_rnm);
        if (ex_df != 0) {
            std::printf(" food=%+d", static_cast<int>(ex_df));
        }
        if (ex_dp != 0) {
            std::printf(" prod=%+d", static_cast<int>(ex_dp));
        }
        if (ex_dc != 0) {
            std::printf(" comm=%+d", static_cast<int>(ex_dc));
        }
        std::printf("\n");
        for (u16 ri = 0; ri < res_n; ++ri) {
            if (res_hit[ri] == 0u) {
                continue;
            }
            cstr rnm = st.resource().get_name(ResourceStaticDataKey::from_raw(ri));
            if (rnm == nullptr) {
                rnm = "?";
            }
            const ResourceStaticDataStruct& row = st.resource().get_item(ResourceStaticDataKey::from_raw(ri));
            std::printf("   %s", rnm);
            if (row.food != 0u) {
                std::printf(" food=%u", static_cast<unsigned>(row.food));
            }
            if (row.shields != 0u) {
                std::printf(" prod=%u", static_cast<unsigned>(row.shields));
            }
            if (row.commerce != 0u) {
                std::printf(" comm=%u", static_cast<unsigned>(row.commerce));
            }
            std::printf("\n");
        }
    }

    const u64 ns_per_get = (sweep_calls == 0u) ? 0u : (sweep_ns / sweep_calls);
    std::printf("-----------------------------------------------------------\n");
    std::printf("TECH SWEEP TIMING\n");
    std::printf(" techs=%u tech_hits=%u calls=%llu total_ns=%llu ns_per_get=%llu\n",
        static_cast<unsigned>(tech_n),
        static_cast<unsigned>(hit_n),
        static_cast<unsigned long long>(sweep_calls),
        static_cast<unsigned long long>(sweep_ns),
        static_cast<unsigned long long>(ns_per_get));
    note_result(hit_n > 0u, "at least one tech gates a resource yield");

    std::printf("=======================================================\n");
    std::printf(" TESTING RES TECH YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return (total_test_fails > 0) ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
