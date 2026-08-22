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
#include "tile_usage.h"
#include "tile_imp_helper.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "worker_guidance.h"
#include "worker_job_enum.h"
#include "worker_job_imp_static_key.h"

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

static bool find_empty_plains (const GameArraySimple& map, u16* ox, u16* oy) {
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

static cstr imp_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.worker_job_imp().get_item_count()) {
        return "?";
    }
    return st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(idx));
}

static void cert_has_pending (u16 x, u16 y, TileAssignIntent intent, cstr tag) {
    u16 job = 0;
    u16 imp = 0;
    const bool nw = WorkerGuidance::next_work(x, y, intent, &job, &imp);
    const bool hp = WorkerGuidance::has_pending_work(x, y, intent);
    note_result(nw == hp, tag);
}

static void test_farm_work_chain (const RuntimeStatics& st, GameArraySimple& map) {
    u16 x = 0;
    u16 y = 0;
    if (!find_empty_plains(map, &x, &y)) {
        note_result(false, "guidance: find empty plains");
        return;
    }
    note_result(true, "guidance: find empty plains");
    map.tile(x, y)->m_riv = 1u;
    cert_has_pending(x, y, TILE_ASSIGN_FOOD, "guidance: has_pending empty plains food");
    u16 job = 0;
    u16 imp = 0;
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_FOOD, &job, &imp), "guidance: next on empty plains");
    note_result(job == static_cast<u16>(WorkerJob::Cultivate_Farm), "guidance: empty plains -> Cultivate Farm");
    note_result(imp == U16_KEY_NULL, "guidance: empty plains -> no imp");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply cultivate");
    note_result(map.get_overlay(x, y) == static_cast<u16>(MapOverlay::Farm), "guidance: tile is Farm");
    cert_has_pending(x, y, TILE_ASSIGN_FOOD, "guidance: has_pending on Farm");
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_FOOD, &job, &imp), "guidance: next on Farm");
    note_result(job == static_cast<u16>(WorkerJob::Cultivate_Farm), "guidance: Farm -> mother job");
    note_result(imp != U16_KEY_NULL, "guidance: Farm -> imp offered");
    if (print_level > 0 && imp != U16_KEY_NULL) {
        std::printf("  imp pick: [%u] %s\n", static_cast<unsigned>(imp), imp_nm(st, imp));
    }
    note_result(std::strcmp(imp_nm(st, imp), "Irrigation") == 0, "guidance: first imp is Irrigation");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply Irrigation");
    note_result(StdAddHelper::has_irr(map.tile(x, y)), "guidance: Irrigation bit set");
    cert_has_pending(x, y, TILE_ASSIGN_FOOD, "guidance: has_pending after Irrigation");
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_FOOD, &job, &imp), "guidance: next after Irrigation");
    note_result(std::strcmp(imp_nm(st, imp), "Water Mill") == 0, "guidance: second imp is Water Mill");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply Water Mill");
    note_result(StdAddHelper::has_mill(map.tile(x, y)), "guidance: Water Mill bit set on Farm");
    cert_has_pending(x, y, TILE_ASSIGN_FOOD, "guidance: has_pending farm done");
    note_result(!WorkerGuidance::next_work(x, y, TILE_ASSIGN_FOOD, &job, &imp), "guidance: Farm fully improved");
}

static void test_forest_work_chain (const RuntimeStatics& st, GameArraySimple& map) {
    (void)st;
    u16 x = 0;
    u16 y = 0;
    if (!find_empty_plains(map, &x, &y)) {
        note_result(false, "guidance: find empty plains for forest");
        return;
    }
    note_result(true, "guidance: find empty plains for forest");
    u16 job = 0;
    u16 imp = 0;
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_PROD, &job, &imp), "guidance: next prod on empty plains");
    note_result(job == static_cast<u16>(WorkerJob::Plant_Forest), "guidance: empty plains -> Plant Forest");
    note_result(imp == U16_KEY_NULL, "guidance: empty plains -> no imp");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply plant forest");
    note_result(map.get_overlay(x, y) == static_cast<u16>(MapOverlay::Forest), "guidance: tile is Forest");
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_PROD, &job, &imp), "guidance: next prod on Forest");
    note_result(job == static_cast<u16>(WorkerJob::Plant_Forest), "guidance: Forest -> mother job");
    note_result(imp != U16_KEY_NULL, "guidance: Forest -> imp offered");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply first forest imp");
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_PROD, &job, &imp), "guidance: next after first forest imp");
    note_result(imp != U16_KEY_NULL, "guidance: Forest -> second imp offered");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply second forest imp");
    note_result(!WorkerGuidance::next_work(x, y, TILE_ASSIGN_PROD, &job, &imp), "guidance: Forest fully improved");
}

static void test_clear_before_farm (GameArraySimple& map) {
    u16 x = 0;
    u16 y = 0;
    bool got = false;
    for (u16 yy = 0; yy < map.height() && !got; ++yy) {
        for (u16 xx = 0; xx < map.width() && !got; ++xx) {
            if (map.get_terrain(xx, yy) != TERR_PLAINS[0]) {
                continue;
            }
            if (map.get_overlay(xx, yy) != static_cast<u16>(MapOverlay::Forest)) {
                continue;
            }
            if (map.get_res(xx, yy) != U16_KEY_NULL) {
                continue;
            }
            x = xx;
            y = yy;
            got = true;
        }
    }
    note_result(got, "guidance: find forested plains");
    if (!got) {
        return;
    }
    u16 job = 0;
    u16 imp = 0;
    note_result(WorkerGuidance::next_work(x, y, TILE_ASSIGN_FOOD, &job, &imp), "guidance: next on forest plains");
    note_result(job == static_cast<u16>(WorkerJob::Clear_Forest), "guidance: forest plains -> Clear Forest");
    note_result(imp == U16_KEY_NULL, "guidance: clear -> no imp");
    note_result(WorkerGuidance::apply_work(x, y, job, imp), "guidance: apply clear");
    note_result(map.get_overlay(x, y) == U16_KEY_NULL, "guidance: forest cleared");
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
        std::printf(" WORKER GUIDANCE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
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
    WorkerGuidance::bind_statics(&st);
    WorkerGuidance::bind_map(&map);
    TileImpHelper::bind_statics(&st);
    test_farm_work_chain(st, map);
    test_forest_work_chain(st, map);
    test_clear_before_farm(map);
    std::printf("=======================================================\n");
    std::printf(" WORKER GUIDANCE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
