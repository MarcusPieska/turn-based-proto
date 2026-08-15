//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "factory_game_array_simple.h"
#include "runtime_static_loader.h"
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

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    note_result(build_paths(), "build map paths");
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING TILE YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    RuntimeStatics& st = loader.statics();
    note_result(TileYields::setup(st), "setup tile yields");

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TESTING TILE YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }

    TileYields::bind_map(&map);
    const u16 w = map.width();
    const u16 h = map.height();
    const u64 tile_n = static_cast<u64>(map.tile_n());
    note_result(TileYields::in_bounds(0, 0), "in_bounds origin");
    note_result(!TileYields::in_bounds(w, h), "oob rejected");
    note_result(tile_n > 0u, "map has tiles");

    u64 warm_acc = 0;
    for (u32 p = 0; p < G_WARM_PASSES; ++p) {
        warm_acc += sweep_map(map);
    }
    note_result(warm_acc > 0u, "warm sweep nonzero yields");

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
    note_result(timed_acc > 0u, "timed sweep nonzero yields");
    note_result(call_n > 0u, "timed call count");

    std::printf("-----------------------------------------------------------\n");
    std::printf("TILE YIELDS TIMING (land attrs + job gate; no farms on map)\n");
    std::printf(" map=%ux%u tiles=%llu warm_passes=%u time_passes=%u\n",
        static_cast<unsigned>(w), static_cast<unsigned>(h),
        static_cast<unsigned long long>(tile_n),
        static_cast<unsigned>(G_WARM_PASSES),
        static_cast<unsigned>(G_TIME_PASSES));
    std::printf(" calls=%llu total_ns=%llu total_ms=%.3f\n",
        static_cast<unsigned long long>(call_n),
        static_cast<unsigned long long>(total_ns),
        ms_total);
    std::printf(" ns_per_get=%llu checksum=%llu\n",
        static_cast<unsigned long long>(ns_per_get),
        static_cast<unsigned long long>(timed_acc));
    std::printf("-----------------------------------------------------------\n");

    TileYields::bind_map(nullptr);

    std::printf("=======================================================\n");
    std::printf(" TESTING TILE YIELDS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
