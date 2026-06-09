//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "map_loader.h"
#include "starting_point_generator.h"

#include "starting_point_generator_tester_helpers.cpp"
#include "starting_point_generator_tester_save.cpp"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

typedef const char* cstr;

#define SEED_BATCH_DEF 10000

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";

static const char* g_out_dir =
    "/home/w/Projects/rts-proto/game_engine_dev/map_loader";

static double secs_since (const std::chrono::steady_clock::time_point& t0) {
    const auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

static int run_once (cstr map_path, cstr out_dir) {
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(map_path, map)) {
        std::printf("*** FAILED load map: %s\n", map_path);
        return 1;
    }
    StartingPointGeneratorParams par = {};
    par.map = &map;
    par.pick_n = 255;
    StartingPointGenerator gen(par);
    if (!gen.generate()) {
        std::printf("*** FAILED generate\n");
        return 1;
    }
    if (!validate_results(gen)) {
        return 1;
    }
    if (!save_mk3_outputs(out_dir, gen)) {
        std::printf("*** FAILED save outputs\n");
        return 1;
    }
    std::printf("*** Map: %s\n", map_path);
    std::printf("*** Dist: max u16=%u\n", gen.dist_max());
    std::printf("*** Pts:  %u lattice, %u plains/hills, %u after prob cull (min %u)\n",
        gen.lattice_count(), gen.land_count(), gen.candidate_count(), par.pick_n);
    if (gen.pick_skipped()) {
        std::printf("*** Pick: %u from %u candidates, skipped (candidates <= target)\n",
            gen.pick_count(), gen.candidate_count());
    } else {
        std::printf("*** Pick: %u from %u candidates, spaced max-min\n",
            gen.pick_count(), gen.candidate_count());
    }
    std::printf("*** Out: %s/DEL_mk3_00_water_dist.ppm\n", out_dir);
    std::printf("*** Out: %s/DEL_mk3_01_prob_cand.ppm\n", out_dir);
    std::printf("*** Out: %s/DEL_mk3_02_pick.ppm\n", out_dir);
    return 0;
}

static int run_seed_batch (cstr map_path, u32 gen_n) {
    if (gen_n == 0) {
        std::printf("*** FAILED batch: gen count must be > 0\n");
        return 1;
    }
    double* times = new double[gen_n];
    double sum = 0.0;
    u32 fail_n = 0;
    MapTerrainData map;
    if (!MapLoader::load_terrain_ppm(map_path, map)) {
        std::printf("*** FAILED load map: %s\n", map_path);
        return 1;
    }
    for (u32 seed = 0; seed < gen_n; ++seed) {
        StartingPointGeneratorParams par = {};
        par.map = &map;
        par.pick_n = 255;
        par.seed = seed;
        StartingPointGenerator gen(par);
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = gen.generate();
        times[seed] = secs_since(t0);
        sum += times[seed];
        if (!ok) {
            fail_n += 1u;
            std::printf("\n*** FAILED seed %u\n", seed);
        } else if (!validate_results(gen)) {
            fail_n += 1u;
        }
        std::printf("\r*** Generating: %u / %u", seed + 1u, gen_n);
        std::fflush(stdout);
    }
    std::printf("\n");
    const double avg = sum / static_cast<double>(gen_n);
    std::printf("*** Batch: %u seeds 0 ... %u\n", gen_n, gen_n - 1u);
    std::printf("*** Failures: %u\n", fail_n);
    std::printf("*** Average time: %.1f ms\n", avg * 1000.0);
    std::printf("*** Total time: %.4f s\n", sum);
    delete[] times;
    return fail_n > 0u ? 1 : 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc <= 1) {
        return run_once(g_map_path, g_out_dir);
    }
    char* end = nullptr;
    const unsigned long n = strtoul(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || n == 0ul) {
        std::printf("*** usage: %s [batch_size] [map.ppm]\n", argv[0]);
        return 1;
    }
    cstr map_path = g_map_path;
    if (argc > 2) {
        map_path = argv[2];
    }
    return run_seed_batch(map_path, static_cast<u32>(n));
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
