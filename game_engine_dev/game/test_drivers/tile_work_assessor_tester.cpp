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
#include "item_reqs.h"
#include "map_overlay_enum.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "std_add_helper.h"
#include "tech_static_key.h"
#include "tile_imp_helper.h"
#include "tile_attr_tables.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
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
static const u32 G_WARM_PASSES = 2u;
static const u32 G_TIME_PASSES = 20u;
static const u16 G_CAND_CAP = 64u;

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

static cstr job_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.worker_job().get_item_count()) {
        return "?";
    }
    return st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(idx));
}

static cstr imp_nm (const RuntimeStatics& st, u16 idx) {
    if (idx == U16_KEY_NULL) {
        return "(job)";
    }
    if (idx >= st.worker_job_imp().get_item_count()) {
        return "?";
    }
    return st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(idx));
}

static cstr tech_nm (const RuntimeStatics& st, u16 idx) {
    if (idx >= st.tech().get_item_count()) {
        return "?";
    }
    return st.tech().get_name(TechStaticDataKey::from_raw(idx));
}

static bool cand_has (const TileWorkCand* cands, u16 n, u16 job_idx, u16 imp_idx) {
    for (u16 i = 0; i < n; ++i) {
        if (cands[i].m_job == job_idx && cands[i].m_imp == imp_idx) {
            return true;
        }
    }
    return false;
}

static void collect_tech_reqs (const ItemReqsStruct& reqs, std::vector<u16>* out) {
    for (u16 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        if (reqs.types[i] == ITEM_REQ_TYPE_NONE) {
            break;
        }
        if (reqs.types[i] == ITEM_REQ_TYPE_TECH && reqs.indices[i] != U16_KEY_NULL) {
            out->push_back(reqs.indices[i]);
        }
    }
}

static bool vec_has (const std::vector<u16>& v, u16 x) {
    for (u32 i = 0; i < v.size(); ++i) {
        if (v[i] == x) {
            return true;
        }
    }
    return false;
}

static void expected_techs (const RuntimeStatics& st, u16 job_idx, u16 imp_idx, std::vector<u16>* out) {
    out->clear();
    std::vector<u16> raw;
    collect_tech_reqs(st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(job_idx)).reqs, &raw);
    if (imp_idx != U16_KEY_NULL) {
        collect_tech_reqs(st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(imp_idx)).reqs, &raw);
    }
    for (u32 i = 0; i < raw.size(); ++i) {
        if (!vec_has(*out, raw[i])) {
            out->push_back(raw[i]);
        }
    }
}

struct WorkTarget {
    u16 m_job;
    u16 m_imp;
    u16 m_x;
    u16 m_y;
    bool m_has_tile;
};

static bool cand_has_imp_for_job (const TileWorkCand* cands, u16 n, u16 job_idx) {
    for (u16 i = 0; i < n; ++i) {
        if (cands[i].m_job == job_idx && cands[i].m_imp != U16_KEY_NULL) {
            return true;
        }
    }
    return false;
}

static bool find_farm_place (const GameArraySimple& map, u16* ox, u16* oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
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

static void test_overlay_imp_gate (const RuntimeStatics& st, GameArraySimple& map) {
    const u16 farm_job = static_cast<u16>(WorkerJob::Cultivate_Farm);
    TileWorkCand cands[G_CAND_CAP];
    u16 px = 0;
    u16 py = 0;
    if (!find_farm_place(map, &px, &py)) {
        note_result(false, "overlay gate: find empty plains tile");
        return;
    }
    note_result(true, "overlay gate: find empty plains tile");
    const u16 n0 = TileWorkAssessor::assess_job(px, py, farm_job, cands, G_CAND_CAP);
    note_result(cand_has(cands, n0, farm_job, U16_KEY_NULL), "overlay gate: placement offers job-only");
    note_result(!cand_has_imp_for_job(cands, n0, farm_job), "overlay gate: placement offers no imps");
    note_result(map.set_overlay(px, py, static_cast<u16>(MapOverlay::Farm)), "overlay gate: stamp Farm");
    const u16 n1 = TileWorkAssessor::assess_job(px, py, farm_job, cands, G_CAND_CAP);
    note_result(n1 > 0, "overlay gate: Farm tile offers imps");
    note_result(!cand_has(cands, n1, farm_job, U16_KEY_NULL), "overlay gate: Farm tile skips job-only");
    u16 forest_job = U16_KEY_NULL;
    const u16 job_n = st.worker_job().get_item_count();
    for (u16 j = 0; j < job_n; ++j) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (row.target_kind == static_cast<u16>(WorkerJobTarget::Overlay)
            && row.target_idx == static_cast<u16>(MapOverlay::Forest)
            && row.type != static_cast<u16>(WorkerJobType::Clearing)) {
            forest_job = j;
            break;
        }
    }
    note_result(forest_job != U16_KEY_NULL, "overlay gate: find forest mother job");
    if (forest_job == U16_KEY_NULL) {
        return;
    }
    u16 fx = 0;
    u16 fy = 0;
    bool got_f = false;
    for (u16 y = 0; y < map.height() && !got_f; ++y) {
        for (u16 x = 0; x < map.width() && !got_f; ++x) {
            if (map.get_overlay(x, y) != static_cast<u16>(MapOverlay::Forest)) {
                continue;
            }
            fx = x;
            fy = y;
            got_f = true;
        }
    }
    note_result(got_f, "overlay gate: find natural Forest tile");
    if (!got_f) {
        return;
    }
    const u16 nf = TileWorkAssessor::assess_job(fx, fy, forest_job, cands, G_CAND_CAP);
    note_result(nf > 0, "overlay gate: Forest tile offers imps");
    note_result(!cand_has(cands, nf, forest_job, U16_KEY_NULL), "overlay gate: Forest tile skips job-only");
}

//================================================================================================================================
//=> - Tech ablation via assessor -
//================================================================================================================================

static void build_targets (const RuntimeStatics& st, const GameArraySimple& map, BitArrayCL& tech, std::vector<WorkTarget>* out) {
    out->clear();
    const u16 job_n = st.worker_job().get_item_count();
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    for (u16 j = 0; j < job_n; ++j) {
        const WorkerJobStaticDataStruct& job = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (job.target_kind != static_cast<u16>(WorkerJobTarget::Overlay)
            || job.type == static_cast<u16>(WorkerJobType::Clearing)) {
            WorkTarget t;
            t.m_job = j;
            t.m_imp = U16_KEY_NULL;
            t.m_x = 0;
            t.m_y = 0;
            t.m_has_tile = false;
            out->push_back(t);
            continue;
        }
        const u16 ov = job.target_idx;
        const u16 imp_n = ix.imp_n(ov);
        if (imp_n == 0) {
            WorkTarget t;
            t.m_job = j;
            t.m_imp = U16_KEY_NULL;
            t.m_x = 0;
            t.m_y = 0;
            t.m_has_tile = false;
            out->push_back(t);
            continue;
        }
        const u16* imps = ix.imps(ov);
        for (u16 i = 0; i < imp_n; ++i) {
            WorkTarget t;
            t.m_job = j;
            t.m_imp = imps[i];
            t.m_x = 0;
            t.m_y = 0;
            t.m_has_tile = false;
            out->push_back(t);
        }
    }
    fill_bits(tech);
    TileWorkCand cands[G_CAND_CAP];
    const u16 w = map.width();
    const u16 h = map.height();
    u32 found = 0;
    const u32 need = static_cast<u32>(out->size());
    for (u16 y = 0; y < h && found < need; ++y) {
        for (u16 x = 0; x < w && found < need; ++x) {
            const u16 n = TileWorkAssessor::assess(x, y, cands, G_CAND_CAP);
            for (u32 ti = 0; ti < out->size(); ++ti) {
                WorkTarget& t = (*out)[ti];
                if (t.m_has_tile) {
                    continue;
                }
                if (!cand_has(cands, n, t.m_job, t.m_imp)) {
                    continue;
                }
                t.m_x = x;
                t.m_y = y;
                t.m_has_tile = true;
                ++found;
            }
        }
    }
}

static void ablate_techs (const RuntimeStatics& st, BitArrayCL& tech, std::vector<WorkTarget>& targets) {
    const u16 tech_n = st.tech().get_item_count();
    TileWorkCand cands[G_CAND_CAP];
    u32 ok_n = 0;
    u32 miss_tile_n = 0;
    u32 mismatch_n = 0;
    std::printf("-----------------------------------------------------------\n");
    std::printf("TECH ABLATION (all techs on -> find tile -> clear one tech)\n");
    for (u32 ti = 0; ti < targets.size(); ++ti) {
        WorkTarget& t = targets[ti];
        char label[192];
        std::snprintf(label, sizeof(label), "%s / %s", job_nm(st, t.m_job), imp_nm(st, t.m_imp));
        if (!t.m_has_tile) {
            ++miss_tile_n;
            std::printf("  %s: SKIP NO_TILE (placement; tech ablation N/A)\n", label);
            continue;
        }
        std::vector<u16> expect;
        expected_techs(st, t.m_job, t.m_imp, &expect);
        std::vector<u16> inferred;
        fill_bits(tech);
        for (u16 tech_idx = 0; tech_idx < tech_n; ++tech_idx) {
            tech.clear_bit(tech_idx);
            const u16 n = TileWorkAssessor::assess_job(t.m_x, t.m_y, t.m_job, cands, G_CAND_CAP);
            const bool still = cand_has(cands, n, t.m_job, t.m_imp);
            tech.set_bit(tech_idx);
            if (!still) {
                inferred.push_back(tech_idx);
            }
        }
        bool match = (inferred.size() == expect.size());
        if (match) {
            for (u32 i = 0; i < inferred.size(); ++i) {
                if (!vec_has(expect, inferred[i])) {
                    match = false;
                    break;
                }
            }
        }
        if (match) {
            ++ok_n;
        } else {
            ++mismatch_n;
        }
        std::printf("  %s @(%u,%u)\n", label, t.m_x, t.m_y);
        std::printf("    inferred:");
        if (inferred.empty()) {
            std::printf(" (none)");
        }
        for (u32 i = 0; i < inferred.size(); ++i) {
            std::printf(" %s", tech_nm(st, inferred[i]));
        }
        std::printf("\n    catalog :");
        if (expect.empty()) {
            std::printf(" (none)");
        }
        for (u32 i = 0; i < expect.size(); ++i) {
            std::printf(" %s", tech_nm(st, expect[i]));
        }
        std::printf("\n");
        note_result(match, label);
    }
    std::printf("  summary ok=%u mismatch=%u no_tile=%u targets=%u\n",
        ok_n, mismatch_n, miss_tile_n, static_cast<u32>(targets.size()));
}

static void time_assess (const GameArraySimple& map) {
    TileWorkCand cands[G_CAND_CAP];
    const u16 w = map.width();
    const u16 h = map.height();
    u64 total_cands = 0;
    u32 nonempty = 0;
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
                const u16 n = TileWorkAssessor::assess(x, y, cands, G_CAND_CAP);
                total_cands += n;
                if (n > 0) {
                    ++nonempty;
                }
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const u64 tiles = static_cast<u64>(w) * static_cast<u64>(h) * static_cast<u64>(G_TIME_PASSES);
    std::printf("-----------------------------------------------------------\n");
    std::printf("TIMING assess (mk01 full scan)\n");
    std::printf("  map %ux%u passes=%u tiles=%llu\n", w, h, G_TIME_PASSES, static_cast<unsigned long long>(tiles));
    std::printf("  wall_ms=%.3f  us/tile=%.3f\n", ms, (ms * 1000.0) / static_cast<double>(tiles));
    std::printf("  cand_sum=%llu  nonempty_tile_hits=%u\n",
        static_cast<unsigned long long>(total_cands), nonempty);
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
        std::printf(" TILE WORK ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    note_result(TileAttrTables::setup(st), "TileAttrTables::setup");
    note_result(TileYields::setup(st), "TileYields::setup");
    note_result(TileWorkAssessor::setup(st), "TileWorkAssessor::setup");
    TileImpHelper::bind_statics(&st);
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" TILE WORK ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    BitArrayCL tech(st.tech().get_item_count());
    BitArrayCL res(st.resource().get_item_count());
    fill_bits(tech);
    fill_bits(res);
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileWorkCtx ctx = {};
    ctx.m_tech = &tech;
    ctx.m_resource = &res;
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    TileWorkAssessor::bind_map(&map);
    TileWorkAssessor::bind_ctx(&ctx);

    test_overlay_imp_gate(st, map);

    std::vector<WorkTarget> targets;
    build_targets(st, map, tech, &targets);
    ablate_techs(st, tech, targets);
    time_assess(map);

    std::printf("=======================================================\n");
    std::printf(" TILE WORK ASSESSOR: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
