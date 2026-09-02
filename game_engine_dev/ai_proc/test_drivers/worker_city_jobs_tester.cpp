//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_ai_helpers.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"
#include "worker_city_jobs.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/worker-city-jobs";
static const u32 G_SEED = 43u;
static const u16 G_PICK_N = 10u;
static const u16 G_LATT_DIV = 10u;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;
static const u32 G_BLEND_A = 160u;

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

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / G_LATT_DIV;
    u16 c = w / G_LATT_DIV;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS,
        TERR_MOUNTAINS, TERR_VOLCANO, TERR_INLAND_SEA, TERR_INLAND_LAKE};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == cls) {
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

static void blend_rgb (u8* r, u8* g, u8* b, u8 tr, u8 tg, u8 tb, u32 a) {
    const u32 ia = 255u - a;
    *r = static_cast<u8>((ia * static_cast<u32>(*r) + a * static_cast<u32>(tr)) / 255u);
    *g = static_cast<u8>((ia * static_cast<u32>(*g) + a * static_cast<u32>(tg)) / 255u);
    *b = static_cast<u8>((ia * static_cast<u32>(*b) + a * static_cast<u32>(tb)) / 255u);
}

static bool write_ppm (cstr path, const std::vector<u8>& rgb, u16 w, u16 h) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static void base_map_rgb (const GameArraySimple& map, u16 x, u16 y, u8* r, u8* g, u8* b) {
    terr_rgb(map.get_terrain(x, y), r, g, b);
    if (!overlay_is_water_terr(map.get_terrain(x, y)) && map.get_river(x, y) != 0u) {
        *r = G_RIV_R;
        *g = G_RIV_G;
        *b = G_RIV_B;
    }
}

static bool write_jobs_ppm (cstr path, const GameArraySimple& map, const WorkerCityJobs& jobs) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> marked(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    for (u32 s = 0; s < jobs.site_n(); ++s) {
        const u16 n = jobs.job_n(s);
        for (u16 i = 0; i < n; ++i) {
            u16 x = 0;
            u16 y = 0;
            if (!jobs.job_at(s, i, &x, &y)) {
                continue;
            }
            marked[static_cast<u32>(y) * w + x] = 1u;
        }
    }
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            base_map_rgb(map, x, y, &r, &g, &b);
            if (map.get_planned_city(x, y) != 0u) {
                blend_rgb(&r, &g, &b, 0u, 255u, 255u, G_BLEND_A);
            }
            if (marked[static_cast<u32>(y) * w + x] != 0u) {
                r = 255u;
                g = 255u;
                b = 0u;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
}

static double ms_since (std::chrono::steady_clock::time_point t0) {
    const auto dt = std::chrono::steady_clock::now() - t0;
    return std::chrono::duration<double, std::milli>(dt).count();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    (void)argc;
    (void)argv;
    note_result(build_paths(), "build_paths");
    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");

    const u16 mw = map.width();
    const u16 mh = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> clim(tn);
    std::vector<u8> ov(tn);
    std::vector<u8> riv(tn);
    for (u16 y = 0; y < mh; ++y) {
        for (u16 x = 0; x < mw; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(mw) + static_cast<u32>(x);
            clim[i] = map.get_climate(x, y);
            ov[i] = map.get_overlay(x, y);
            riv[i] = map.get_river(x, y);
        }
    }
    u16 latt_r = 0;
    u16 latt_c = 0;
    latt_for_map(mw, mh, &latt_r, &latt_c);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.climate = clim.data();
    par.overlay = ov.data();
    par.river = riv.data();
    par.pick_n = G_PICK_N;
    par.latt_rows = latt_r;
    par.latt_cols = latt_c;
    par.seed = G_SEED;
    StartingPointGenerator spg(par);
    note_result(spg.generate(), "generate starting points");
    const SpgPickCoords starts = spg.picks_coords();

    WhiteboardMng::init(mw, mh);
    note_result(WhiteboardMng::width() == mw, "whiteboard init");

    GenAiHelpers gah;
    note_result(gah.begin(map), "GenAiHelpers::begin");
    GenAiHelpersRslt rslt = {};
    const auto t_help0 = std::chrono::steady_clock::now();
    const bool build_ok = gah.build(starts.pts, starts.n, &rslt);
    const double help_ms = ms_since(t_help0);
    note_result(build_ok, "GenAiHelpers::build");

    WorkerCityJobs jobs;
    const auto t_jobs0 = std::chrono::steady_clock::now();
    const bool jobs_ok = jobs.build(map);
    const double jobs_ms = ms_since(t_jobs0);
    note_result(jobs_ok, "WorkerCityJobs::build");
    note_result(jobs.ok(), "jobs ok");
    note_result(jobs.site_n() > 0u, "sites > 0");
    u32 job_n = 0;
    for (u32 i = 0; i < jobs.site_n(); ++i) {
        job_n += jobs.job_n(i);
    }
    note_result(job_n > 0u, "jobs > 0");
    note_result(jobs.site_n() == rslt.m_wcj_site_n || jobs.site_n() == rslt.m_city_n, "site count sane");

    char ppm_path[384];
    std::snprintf(ppm_path, sizeof(ppm_path), "%s/worker_city_jobs.ppm", G_OUT_DIR);
    ::mkdir(G_OUT_DIR, 0755);
    note_result(write_jobs_ppm(ppm_path, map, jobs), ppm_path);

    std::printf("=======================================================\n");
    std::printf(" WORKER CITY JOBS: %s\n", total_test_fails == 0 ? "PASS" : "FAIL");
    std::printf(" helpers build: %.3f ms\n", help_ms);
    std::printf(" jobs build:    %.3f ms  sites=%u jobs=%u\n",
        jobs_ms, (unsigned)jobs.site_n(), (unsigned)job_n);
    std::printf(" helpers rslt:  city=%u wcj_sites=%u wcj_jobs=%u\n",
        (unsigned)rslt.m_city_n, (unsigned)rslt.m_wcj_site_n, (unsigned)rslt.m_wcj_job_n);
    std::printf(" ppm: %s\n", ppm_path);
    std::printf("=======================================================\n");

    gah.clr();
    WhiteboardMng::terminate();
    return total_test_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
