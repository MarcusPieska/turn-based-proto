//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "bit_array.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "worker_imp_select.h"
#include "worker_job_enum.h"
#include "worker_job_imp_index.h"
#include "worker_job_imp_static_key.h"
#include "worker_job_static_data.h"
#include "worker_job_static_key.h"
#include "worker_job_target_enum.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;

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

static void fill_bits (BitArrayCL& ba) {
    const u32 n = ba.get_count();
    for (u32 i = 0; i < n; ++i) {
        ba.set_bit(i);
    }
}

static bool find_farm_place (const GameArraySimple& map, u16* ox, u16* oy) {
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (map.get_terrain(x, y) != TERR_PLAINS[0]) {
                continue;
            }
            if (map.get_overlay(x, y) != U16_KEY_NULL) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static u16 forest_mjob (const RuntimeStatics& st) {
    const u16 job_n = st.worker_job().get_item_count();
    for (u16 j = 0; j < job_n; ++j) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (row.target_kind == static_cast<u16>(WorkerJobTarget::Overlay)
            && row.target_idx == static_cast<u16>(MapOverlay::Forest)
            && row.type != static_cast<u16>(WorkerJobType::Clearing)) {
            return j;
        }
    }
    return U16_KEY_NULL;
}

static u16 first_farm_imp (const RuntimeStatics& st) {
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 ov = static_cast<u16>(MapOverlay::Farm);
    if (ix.imp_n(ov) == 0) {
        return U16_KEY_NULL;
    }
    return ix.imps(ov)[0];
}

static u16 second_farm_imp (const RuntimeStatics& st) {
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 ov = static_cast<u16>(MapOverlay::Farm);
    if (ix.imp_n(ov) < 2) {
        return U16_KEY_NULL;
    }
    return ix.imps(ov)[1];
}

static cstr imp_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.worker_job_imp().get_item_count()) {
        return "?";
    }
    return st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(idx));
}

static void test_linear_pick (const RuntimeStatics& st, GameArraySimple& map) {
    const u16 farm_job = static_cast<u16>(WorkerJob::Cultivate_Farm);
    u16 px = 0;
    u16 py = 0;
    if (!find_farm_place(map, &px, &py)) {
        note_result(false, "linear: find empty plains tile");
        return;
    }
    note_result(true, "linear: find empty plains tile");
    note_result(WorkerImpSelect::pick(px, py, farm_job) == U16_KEY_NULL, "linear: empty tile picks no imp");
    note_result(map.set_overlay(px, py, static_cast<u16>(MapOverlay::Farm)), "linear: stamp Farm");
    const u16 first = first_farm_imp(st);
    const u16 second = second_farm_imp(st);
    note_result(first != U16_KEY_NULL, "linear: farm imp catalog");
    note_result(second != U16_KEY_NULL, "linear: farm has second imp");
    const u16 p0 = WorkerImpSelect::pick(px, py, farm_job);
    note_result(p0 == first, "linear: first pick is catalog head");
    if (print_level > 0) {
        std::printf("  first pick: [%u] %s\n", static_cast<unsigned>(p0), imp_nm(st, p0));
    }
    StdAddHelper::set_irr(map.tile(px, py));
    const u16 p1 = WorkerImpSelect::pick(px, py, farm_job);
    note_result(p1 == second, "linear: skips set imp");
    if (print_level > 0) {
        std::printf("  second pick: [%u] %s\n", static_cast<unsigned>(p1), imp_nm(st, p1));
    }
    const u16 fj = forest_mjob(st);
    note_result(fj != U16_KEY_NULL, "linear: forest mother job");
    if (fj == U16_KEY_NULL) {
        return;
    }
    u16 fx = 0;
    u16 fy = 0;
    bool got = false;
    for (u16 y = 0; y < map.height() && !got; ++y) {
        for (u16 x = 0; x < map.width() && !got; ++x) {
            if (map.get_overlay(x, y) != static_cast<u16>(MapOverlay::Forest)) {
                continue;
            }
            fx = x;
            fy = y;
            got = true;
        }
    }
    note_result(got, "linear: find natural Forest tile");
    if (!got) {
        return;
    }
    const u16 fp = WorkerImpSelect::pick(fx, fy, fj);
    note_result(fp != U16_KEY_NULL, "linear: forest tile picks imp");
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16* imps = ix.imps(static_cast<u16>(MapOverlay::Forest));
    note_result(fp == imps[0], "linear: forest pick is catalog head");
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
    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" WORKER IMP SELECT: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    note_result(TileYields::setup(st), "TileYields::setup");
    note_result(TileWorkAssessor::setup(st), "TileWorkAssessor::setup");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");
    BitArrayCL tech(st.tech().get_item_count());
    BitArrayCL res(st.resource().get_item_count());
    fill_bits(tech);
    fill_bits(res);
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = &res;
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    TileWorkAssessor::bind_map(&map);
    TileWorkAssessor::bind_ctx(&wctx);
    test_linear_pick(st, map);
    std::printf("=======================================================\n");
    std::printf(" WORKER IMP SELECT: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
