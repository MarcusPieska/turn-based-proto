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
#include "worker_job_static_key.h"
#include "worker_job_type_enum.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u32 G_WARM_PASSES = 1u;
static const u32 G_TIME_PASSES = 10u;
static const u16 G_CAND_CAP = 64u;
static const int G_DOT_R = 0;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_dir[256];

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
    if (std::snprintf(g_dir, sizeof(g_dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", g_dir) <= 0) {
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

static void slug_spaces (char* s) {
    for (char* p = s; *p; ++p) {
        if (*p == ' ') {
            *p = '_';
        }
    }
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE, TERR_TILE_SENTINEL
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == terr) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static u32 count_ok (u16 job_idx, const GameArraySimple& map, std::vector<u8>* mark) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0;
    if (mark != nullptr) {
        mark->assign(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!TileWorkAssessor::tile_ok(job_idx, x, y)) {
                continue;
            }
            ++n;
            if (mark != nullptr) {
                (*mark)[static_cast<u32>(y) * w + x] = 1u;
            }
        }
    }
    return n;
}

static void time_assess_job (u16 job_idx, const GameArraySimple& map, double* out_ms, double* out_us) {
    TileWorkCand cands[G_CAND_CAP];
    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 p = 0; p < G_WARM_PASSES; ++p) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                TileWorkAssessor::assess_job(x, y, job_idx, cands, G_CAND_CAP);
            }
        }
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (u32 p = 0; p < G_TIME_PASSES; ++p) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                TileWorkAssessor::assess_job(x, y, job_idx, cands, G_CAND_CAP);
            }
        }
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    const u64 tiles = static_cast<u64>(w) * static_cast<u64>(h) * static_cast<u64>(G_TIME_PASSES);
    *out_ms = ms;
    *out_us = (ms * 1000.0) / static_cast<double>(tiles);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 cx, u16 cy) {
    for (int dy = -G_DOT_R; dy <= G_DOT_R; ++dy) {
        for (int dx = -G_DOT_R; dx <= G_DOT_R; ++dx) {
            if (dx * dx + dy * dy > G_DOT_R * G_DOT_R) {
                continue;
            }
            const int x = static_cast<int>(cx) + dx;
            const int y = static_cast<int>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<int>(w) || y >= static_cast<int>(h)) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
            rgb[i] = 220;
            rgb[i + 1] = 30;
            rgb[i + 2] = 30;
        }
    }
}

static bool write_sites_ppm (cstr path, const GameArraySimple& map, const std::vector<u8>& mark) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (mark[static_cast<u32>(y) * w + x] != 0u) {
                paint_dot(rgb, w, h, x, y);
            }
        }
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static void run_mode (cstr mode, u16 job_idx, GameArraySimple& map, cstr ppm_path) {
    std::vector<u8> mark;
    const u32 n = count_ok(job_idx, map, &mark);
    double ms = 0.0;
    double us = 0.0;
    time_assess_job(job_idx, map, &ms, &us);
    std::printf("  [%s] valid=%u  wall_ms=%.3f  us/tile=%.3f\n", mode, n, ms, us);
    note_result(write_sites_ppm(ppm_path, map, mark), ppm_path);
    std::printf("    wrote %s\n", ppm_path);
}

static bool find_land_tile (const GameArraySimple& map, u16* ox, u16* oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (overlay_is_water_terr(map.get_terrain(x, y))) {
                continue;
            }
            *ox = x;
            *oy = y;
            return true;
        }
    }
    return false;
}

static void collect_road_jobs (const RuntimeStatics& st, std::vector<u16>* jobs) {
    jobs->clear();
    const u16 jn = st.worker_job().get_item_count();
    for (u16 j = 0; j < jn; ++j) {
        const WorkerJobStaticDataStruct& row = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(j));
        if (static_cast<WorkerJobType>(row.type) != WorkerJobType::Road) {
            continue;
        }
        jobs->push_back(j);
    }
    for (size_t i = 1; i < jobs->size(); ++i) {
        const u16 key = (*jobs)[i];
        const u16 key_attr = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(key)).target_idx;
        size_t j = i;
        while (j > 0) {
            const u16 prev = (*jobs)[j - 1];
            const u16 prev_attr = st.worker_job().get_item(WorkerJobStaticDataKey::from_raw(prev)).target_idx;
            if (prev_attr <= key_attr) {
                break;
            }
            (*jobs)[j] = prev;
            j = j - 1;
        }
        (*jobs)[j] = key;
    }
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
        std::printf(" ROAD PLACE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();

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
    note_result(TileYields::setup(st), "TileYields::setup");
    TileYields::bind_map(&map);
    TileYields::bind_ctx(&yctx);
    note_result(TileWorkAssessor::setup(st), "TileWorkAssessor::setup");
    TileWorkAssessor::bind_map(&map);
    TileWorkAssessor::bind_ctx(&wctx);

    std::vector<u16> road_jobs;
    collect_road_jobs(st, &road_jobs);
    note_result(road_jobs.size() >= 4u, "found Road jobs");

    for (size_t i = 0; i < road_jobs.size(); ++i) {
        const u16 j = road_jobs[i];
        cstr job_nm = st.worker_job().get_name(WorkerJobStaticDataKey::from_raw(j));
        std::printf("-----------------------------------------------------------\n");
        std::printf("ROAD JOB: %s (idx=%u)\n", job_nm != nullptr ? job_nm : "?", j);
        char ppm[384];
        std::snprintf(ppm, sizeof(ppm), "%s/place_road_%s.ppm", g_dir, job_nm != nullptr ? job_nm : "job");
        slug_spaces(ppm);
        run_mode("ladder", j, map, ppm);
        if (i == 0u) {
            note_result(count_ok(j, map, nullptr) > 0u, "base road job has land sites");
        } else {
            note_result(count_ok(j, map, nullptr) == 0u, "upgrade job has no sites on bare map");
        }
    }

    u16 lx = 0;
    u16 ly = 0;
    note_result(find_land_tile(map, &lx, &ly), "find land tile");
    if (total_test_fails == 0 && road_jobs.size() >= 2u) {
        TileWorkCand cands[G_CAND_CAP];
        const u16 j0 = road_jobs[0];
        const u16 j1 = road_jobs[1];
        note_result(TileWorkAssessor::tile_ok(j0, lx, ly), "base ok on NONE");
        note_result(!TileWorkAssessor::tile_ok(j1, lx, ly), "upgrade blocked on NONE");
        note_result(map.set_road_typ(lx, ly, ROAD_VIRTUAL), "stamp VIRTUAL");
        note_result(TileWorkAssessor::tile_ok(j0, lx, ly), "base ok on VIRTUAL");
        note_result(!TileWorkAssessor::tile_ok(j1, lx, ly), "upgrade blocked on VIRTUAL");
        u8 r0 = ROAD_NONE;
        note_result(TileWorkAssessor::road_result_typ(j0, &r0) && r0 == ROAD_PATH, "base result PATH");
        note_result(map.set_road_typ(lx, ly, r0), "set PATH");
        note_result(!TileWorkAssessor::tile_ok(j0, lx, ly), "base blocked on PATH");
        note_result(TileWorkAssessor::tile_ok(j1, lx, ly), "next upgrade ok on PATH");
        const u16 n1 = TileWorkAssessor::assess_job(lx, ly, j1, cands, G_CAND_CAP);
        note_result(n1 == 1u && cands[0].m_job == j1 && cands[0].m_imp == U16_KEY_NULL, "assess_job next upgrade");
        if (road_jobs.size() >= 3u) {
            const u16 j2 = road_jobs[2];
            note_result(!TileWorkAssessor::tile_ok(j2, lx, ly), "skip-tier blocked on PATH");
            u8 r1 = ROAD_NONE;
            note_result(TileWorkAssessor::road_result_typ(j1, &r1), "mid result typ");
            note_result(map.set_road_typ(lx, ly, r1), "set mid road");
            note_result(TileWorkAssessor::tile_ok(j2, lx, ly), "next ok after mid");
            note_result(!TileWorkAssessor::tile_ok(j1, lx, ly), "mid blocked after built");
        }
    }

    loader.unload();
    std::printf("=======================================================\n");
    std::printf(" ROAD PLACE: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
