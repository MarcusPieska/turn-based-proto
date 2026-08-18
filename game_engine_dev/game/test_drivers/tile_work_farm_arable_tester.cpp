//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "bit_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "worker_job_enum.h"

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
static const u16 G_CAND_CAP = 64u;
static const u16 G_FOOD_BUCKETS = 16u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_ppm_def[320];
static char g_ppm_clr[320];

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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ppm_def, sizeof(g_ppm_def), "%s/arable_farm.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ppm_clr, sizeof(g_ppm_clr), "%s/arable_farm_ov_clear.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static void fill_bits (BitArrayCL& ba) {
    const u32 n = ba.get_count();
    for (u32 i = 0; i < n; ++i) {
        ba.set_bit(i);
    }
}

static void clear_all_ov (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            map.set_overlay(x, y, OV_NONE[0]);
        }
    }
}

struct ArableStats {
    u32 m_n;
    u32 m_by_food[G_FOOD_BUCKETS];
};

static void count_arable (const GameArraySimple& map, u16 farm_job, ArableStats* st) {
    st->m_n = 0;
    for (u16 i = 0; i < G_FOOD_BUCKETS; ++i) {
        st->m_by_food[i] = 0;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!TileWorkAssessor::tile_ok(farm_job, x, y)) {
                continue;
            }
            ++st->m_n;
            const u8 food = TileYields::food_with_job(x, y, farm_job);
            const u16 b = food < G_FOOD_BUCKETS ? food : static_cast<u16>(G_FOOD_BUCKETS - 1u);
            ++st->m_by_food[b];
        }
    }
}

static void pr_stats (cstr label, const ArableStats& st, u32 tiles) {
    std::printf("-----------------------------------------------------------\n");
    std::printf("ARABLE %s\n", label);
    std::printf("  arable=%u / %u (%.2f%%)\n",
        st.m_n, tiles, tiles == 0u ? 0.0 : (100.0 * static_cast<double>(st.m_n) / static_cast<double>(tiles)));
    std::printf("  food_with_farm buckets:\n");
    for (u16 i = 0; i < G_FOOD_BUCKETS; ++i) {
        if (st.m_by_food[i] == 0u) {
            continue;
        }
        if (i + 1u == G_FOOD_BUCKETS) {
            std::printf("    food>=%u: %u\n", i, st.m_by_food[i]);
        } else {
            std::printf("    food=%u: %u\n", i, st.m_by_food[i]);
        }
    }
}

static bool write_arable_ppm (cstr path, const GameArraySimple& map, u16 farm_job) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            if (map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = TERR_MOUNTAINS[1];
                g = TERR_MOUNTAINS[2];
                b = TERR_MOUNTAINS[3];
            }
            if (TileWorkAssessor::tile_ok(farm_job, x, y)) {
                r = 220;
                g = 30;
                b = 30;
            }
            const u8 px[3] = {r, g, b};
            if (std::fwrite(px, 1, 3, f) != 3u) {
                std::fclose(f);
                return false;
            }
        }
    }
    std::fclose(f);
    return true;
}

static void time_assess (const GameArraySimple& map) {
    TileWorkCand cands[G_CAND_CAP];
    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 p = 0; p < G_WARM_PASSES; ++p) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                TileWorkAssessor::assess(x, y, cands, G_CAND_CAP);
            }
        }
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (u32 p = 0; p < G_TIME_PASSES; ++p) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                TileWorkAssessor::assess(x, y, cands, G_CAND_CAP);
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const u64 tiles = static_cast<u64>(w) * static_cast<u64>(h) * static_cast<u64>(G_TIME_PASSES);
    std::printf("-----------------------------------------------------------\n");
    std::printf("TIMING assess (whole map)\n");
    std::printf("  map %ux%u passes=%u tiles=%llu\n", w, h, G_TIME_PASSES, static_cast<unsigned long long>(tiles));
    std::printf("  wall_ms=%.3f  us/tile=%.3f\n", ms, (ms * 1000.0) / static_cast<double>(tiles));
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    note_result(build_paths(), "build map paths");

    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" FARM ARABLE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");

    BitArrayCL tech(st.tech().get_item_count());
    fill_bits(tech);
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = nullptr;

    note_result(TileYields::setup(st), "TileYields::setup");
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    note_result(TileWorkAssessor::setup(st), "TileWorkAssessor::setup");
    TileWorkAssessor::bind_map(&map);
    TileWorkAssessor::bind_ctx(&wctx);

    const u16 farm = static_cast<u16>(WorkerJob::Farm);
    const u32 tiles = map.tile_n();

    ArableStats st_def = {};
    count_arable(map, farm, &st_def);
    pr_stats("default overlays", st_def, tiles);
    note_result(write_arable_ppm(g_ppm_def, map, farm), "write arable_farm.ppm");
    std::printf("  wrote %s\n", g_ppm_def);

    std::vector<u8> ov_bak(tiles);
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            ov_bak[static_cast<u32>(y) * map.width() + x] = map.get_overlay(x, y);
        }
    }
    clear_all_ov(map);
    ArableStats st_clr = {};
    count_arable(map, farm, &st_clr);
    pr_stats("overlays cleared", st_clr, tiles);
    note_result(write_arable_ppm(g_ppm_clr, map, farm), "write arable_farm_ov_clear.ppm");
    std::printf("  wrote %s\n", g_ppm_clr);

    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            map.set_overlay(x, y, ov_bak[static_cast<u32>(y) * map.width() + x]);
        }
    }

    time_assess(map);
    note_result(st_def.m_n > 0u, "default arable > 0");
    note_result(st_clr.m_n >= st_def.m_n, "cleared arable >= default");

    loader.unload();

    std::printf("=======================================================\n");
    std::printf(" FARM ARABLE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
