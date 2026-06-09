//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <errno.h>
#include <sys/stat.h>

#include "generator_constants.h"
#include "map_loader.h"
#include "map_terrain_validate.h"
#include "starting_point_generator.h"
#include "generate_distance_land_to_water.h"
#include "sense_making_settler_pts.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* g_map_path =
    "/home/w/Projects/simple-map-gen/var-seed-outline/combo-min-0.0000-0.6500/frame_028.ppm";

static const char* g_out_path =
    "/home/w/Projects/simple-map-gen/ai-settle-pts/DEL_sms_settler_pts.ppm";

static const u16 k_latt_div = 10;
static const u16 k_pick_n = 10;
static const u16 k_player = 1;
static const u16 k_min_dist = 4;
static const u16 k_max_dist = 6;

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 0;

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

static bool mk_own_map (MapTerrainData* own, u16 w, u16 h) {
    if (own == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* buf = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        buf[i] = 0;
    }
    const bool ok = own->assign_copy(w, h, buf);
    delete[] buf;
    return ok;
}

static bool mk_l2w (const MapTerrainData& map, Generate_DistanceLandToWater& gen) {
    if (map.data() == nullptr || map.width() == 0 || map.height() == 0) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* wl = new u8[n];
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = map.data()[i];
        if (cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]) {
            wl[i] = WL_OVERLAY_WATER_GRAY;
        } else {
            wl[i] = WL_OVERLAY_LAND_GRAY;
        }
    }
    const bool ok = gen.generate(wl, w, h);
    delete[] wl;
    return ok;
}

static i32 l2w_at (
    const MapArrayDistance& l2w,
    const MapTerrainData& map,
    u16 px,
    u16 py) {
    if (l2w.data() == nullptr || l2w.width() != map.width() || l2w.height() != map.height()) {
        return 0;
    }
    const u32 i = static_cast<u32>(py) * static_cast<u32>(map.width()) + static_cast<u32>(px);
    const u16 raw = l2w.data()[i];
    if (raw == 0xFFFFu) {
        return -1;
    }
    return static_cast<i32>(raw);
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

static bool save_sel_ppm (
    cstr path,
    const MapTerrainData& map,
    const SmSettlerPtResult& sel,
    const SpgPickCoords& starts,
    const SmSettlerPt& pick) {
    if (path == nullptr || map.data() == nullptr || map.width() == 0 || map.height() == 0) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(map.data()[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 i = 0; i < sel.n; ++i) {
        set_px_rgb(rgb, w, h, sel.pts[i].x, sel.pts[i].y, 128, 128, 128);
    }
    for (u32 i = 0; i < starts.n; ++i) {
        set_px_rgb(rgb, w, h, starts.pts[i].x, starts.pts[i].y, 0, 0, 0);
    }
    if (pick.x != U16_KEY_NULL) {
        set_px_rgb(rgb, w, h, pick.x, pick.y, 0, 0, 0);
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

void test_sense_making_settler_pts () {
    MapTerrainData map;
    note_result(MapLoader::load_terrain_ppm(g_map_path, map), "load map");

    u16 latt_rows = 0;
    u16 latt_cols = 0;
    latt_for_map(map.width(), map.height(), &latt_rows, &latt_cols);

    StartingPointGeneratorParams par = {};
    par.map = &map;
    par.pick_n = k_pick_n;
    par.latt_rows = latt_rows;
    par.latt_cols = latt_cols;

    StartingPointGenerator gen(par);
    note_result(gen.generate(), "generate starting points");
    note_result(gen.picks_are_start_land(), "starts on settle land");

    SpgPickCoords starts = gen.picks_coords();
    note_result(starts.n == static_cast<u32>(k_pick_n), "start count");

    MapTerrainData own;
    note_result(mk_own_map(&own, map.width(), map.height()), "build ownership map");

    SmSettlerPtResult sel = SenseMakingSettlerPt::select(
        map,
        own,
        k_player,
        k_min_dist,
        k_max_dist,
        starts.pts,
        starts.n);

    note_result(sel.n > 0, "settler points found");

    Generate_DistanceLandToWater l2w_gen(0);
    note_result(mk_l2w(map, l2w_gen), "land-to-water distance overlay");

    SpgCoordPair cap = starts.pts[0];
    SmSettlerPt pick = SenseMakingSettlerPt::pick(map, sel, cap, l2w_gen.distance());
    note_result(pick.x != U16_KEY_NULL, "pick settler point");

    note_result(ensure_dir("/home/w/Projects/simple-map-gen/ai-settle-pts"), "ensure output dir");
    note_result(save_sel_ppm(g_out_path, map, sel, starts, pick), "save settler overlay ppm");

    if (print_level > 0) {
        std::printf("*** Map: %s\n", g_map_path);
        std::printf("*** Starts: %u\n", starts.n);
        std::printf("*** Selected: %u (dist %u..%u, player %u)\n", sel.n, k_min_dist, k_max_dist, k_player);
        std::printf("*** Pick: (%u, %u) from capital (%u, %u)\n", pick.x, pick.y, cap.x, cap.y);
        std::printf("*** Pick l2w cost: %d\n", l2w_at(l2w_gen.distance(), map, pick.x, pick.y));
        std::printf("*** Out: %s\n", g_out_path);
    }

    summarize_test_results();
}

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }

    test_sense_making_settler_pts();

    std::printf("=======================================================\n");
    std::printf(" TESTING SENSE MAKING SETTLER PTS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");

    return total_test_fails;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

