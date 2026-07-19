//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <sys/stat.h>

#include "city.h"
#include "city_array.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "map_loader.h"
#include "sense_settling_pts_opt.h"
#include "starting_point_generator.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* g_map_root = "/home/w/Projects/simple-map-gen/p1-seed-42";
static const char* g_out_path = "/home/w/Projects/simple-map-gen/ai-settle-pts/DEL_sso_settler_pts.ppm";

static const u16 k_latt_div = 10;
static const u16 k_pick_n = 10;
static const u16 k_player = 1;
static const u32 k_samp_cap = 64u;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

//================================================================================================================================
//=> - ChronoFn -
//================================================================================================================================

struct ChronoFn {
    const char* name;
    u64* samp;
    u32 n;
    u64 total_ns;
};

static ChronoFn g_tm_sel = {"SenseSettlingPtsOpt::select_and_pick_pts", nullptr, 0, 0};

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

void summarize_test_results () {
    if (print_level > 0) {
        std::printf("--------------------------------\n");
        std::printf(" Test count: %d\n", test_count);
        std::printf(" Test pass: %d\n", test_pass);
        std::printf(" Test fail: %d\n", test_count - test_pass);
        std::printf("--------------------------------\n\n\n");
    }
    test_count = 0;
    test_pass = 0;
}

static void tm_add (ChronoFn* t, u64 ns) {
    if (t == nullptr || t->samp == nullptr || t->n >= k_samp_cap) {
        return;
    }
    t->samp[t->n] = ns;
    t->n = t->n + 1u;
    t->total_ns = t->total_ns + ns;
}

static void tm_report (const ChronoFn& t) {
    const double total_us = static_cast<double>(t.total_ns) / 1.0e3;
    const double avg_us = (t.n == 0) ? 0.0 : total_us / static_cast<double>(t.n);
    std::printf("  %-40s calls=%u  total=%.2f us  avg=%.2f us\n", t.name, t.n, total_us, avg_us);
}

static bool ensure_dir (cstr path) {
    if (path == nullptr) {
        return false;
    }
    if (mkdir(path, 0755) != 0 && errno != EEXIST) {
        return false;
    }
    return true;
}

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / k_latt_div;
    u16 c = w / k_latt_div;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static void set_px_rgb (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void fill_clim_base (u8* rgb, const GameArraySimple& gmap) {
    const u16 w = gmap.width();
    const u16 h = gmap.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(gmap.get_climate(x, y), &r, &g, &b);
            if (gmap.get_river(x, y) != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            if (gmap.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            set_px_rgb(rgb, w, h, x, y, r, g, b);
        }
    }
}

static bool save_best_ppm (
    cstr path,
    const GameArraySimple& gmap,
    const SpgPickCoords& starts,
    const SmSettlerBestPts& best)
{
    if (path == nullptr || gmap.width() == 0 || gmap.height() == 0) {
        return false;
    }
    const u16 w = gmap.width();
    const u16 h = gmap.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    fill_clim_base(rgb, gmap);
    for (u32 i = 0; i < starts.n; ++i) {
        set_px_rgb(rgb, w, h, starts.pts[i].x, starts.pts[i].y, 0, 0, 0);
    }
    for (u16 i = 0; i < best.n; ++i) {
        set_px_rgb(rgb, w, h, best.pts[i].x, best.pts[i].y, 255, 255, 255);
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void test_sense_settling_pts_opt () {
    char terr_path[320];
    char clim_path[320];
    char riv_path[320];
    note_result(
        std::snprintf(terr_path, sizeof(terr_path), "%s/terrain.ppm", g_map_root) > 0
            && std::snprintf(clim_path, sizeof(clim_path), "%s/climate.ppm", g_map_root) > 0
            && std::snprintf(riv_path, sizeof(riv_path), "%s/rivers.ppm", g_map_root) > 0,
        "build map paths");

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(terr_path, terr), "load terrain");

    GameArraySimple gmap;
    note_result(Factory_GameArraySimple::load_map_gen_data(&gmap, terr_path, clim_path, riv_path, nullptr),
        "load GameArraySimple");

    WhiteboardMng::init(gmap.width(), gmap.height());

    g_tm_sel.samp = new u64[k_samp_cap];
    note_result(g_tm_sel.samp != nullptr, "timing sample buffer");

    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(terr.width(), terr.height(), &latt_rows, &latt_cols);

    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.pick_n = k_pick_n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;

    StartingPointGenerator gen(par);
    note_result(gen.generate(), "generate starting points");
    note_result(gen.picks_are_start_land(), "starts on settle land");

    SpgPickCoords starts = gen.picks_coords();
    note_result(starts.n == static_cast<u32>(k_pick_n), "start count");

    CityArray cities;
    for (u32 i = 0; i < starts.n; ++i) {
        const u16 idx = cities.get_next_new_city_idx();
        note_result(idx != U16_KEY_NULL, "alloc city");
        City* c = cities.get_city(idx);
        note_result(c != nullptr, "city ptr");
        if (c != nullptr) {
            c->init(k_player, starts.pts[i].x, starts.pts[i].y);
            note_result(c->is_frontier(), "city starts frontier");
        }
    }

    const auto ts0 = std::chrono::steady_clock::now();
    const SmSettlerBestPts best = SenseSettlingPtsOpt::select_and_pick_pts(gmap, cities, k_player);
    const auto ts1 = std::chrono::steady_clock::now();
    tm_add(&g_tm_sel, static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(ts1 - ts0).count()));
    note_result(best.n > 0 && best.pts[0].x != U16_KEY_NULL, "pick settler points");

    note_result(ensure_dir("/home/w/Projects/simple-map-gen/ai-settle-pts"), "ensure output dir");
    note_result(save_best_ppm(g_out_path, gmap, starts, best), "save settler overlay ppm");

    if (print_level > 0) {
        std::printf("*** Map root: %s\n", g_map_root);
        std::printf("*** Cities: %u\n", cities.get_city_count());
        std::printf("*** Best n: %u\n", best.n);
        if (best.n > 0) {
            std::printf("*** Pick0: (%u, %u)\n", best.pts[0].x, best.pts[0].y);
        }
        std::printf("*** Out: %s\n", g_out_path);
        std::printf("*** Whiteboard chkout: %u\n", WhiteboardMng::chkout());
    }

    note_result(WhiteboardMng::chkout() == 0, "whiteboard returned");
    tm_report(g_tm_sel);
    delete[] g_tm_sel.samp;
    g_tm_sel.samp = nullptr;
    WhiteboardMng::terminate();
    gmap.clear();
    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_sense_settling_pts_opt();

    std::printf("=======================================================\n");
    std::printf(" TESTING SENSE SETTLING PTS OPT: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails; 
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
