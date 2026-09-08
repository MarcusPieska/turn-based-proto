//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "land_potential.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "runtime_static_loader.h"
#include "starting_point_generator.h"
#include "tile_attr_tables.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/lucky-seat";
static const u32 G_SEED = 101u;
static const u16 G_PICK_N = 100u;
static const u16 G_LATT_DIV = 10u;
static const u16 G_MARK_R = 5u;
static const u16 G_BORDER = 2u;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

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

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / G_LATT_DIV;
    u16 c = w / G_LATT_DIV;
    if (r == 0u) {
        r = 1u;
    }
    if (c == 0u) {
        c = 1u;
    }
    *rows = r;
    *cols = c;
}

static void set_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0u] = r;
    rgb[i + 1u] = g;
    rgb[i + 2u] = b;
}

static void draw_disk (
    u8* rgb,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    u16 rad,
    u8 cr,
    u8 cg,
    u8 cb,
    u16 border)
{
    if (rgb == nullptr || rad == 0u) {
        return;
    }
    const f32 r = static_cast<f32>(rad);
    const f32 b = static_cast<f32>(border);
    const i32 pad = static_cast<i32>(rad + border + 1u);
    const i32 x0 = static_cast<i32>(cx) - pad;
    const i32 x1 = static_cast<i32>(cx) + pad;
    const i32 y0 = static_cast<i32>(cy) - pad;
    const i32 y1 = static_cast<i32>(cy) + pad;
    for (i32 y = y0; y <= y1; ++y) {
        for (i32 x = x0; x <= x1; ++x) {
            const f32 dx = static_cast<f32>(x) + 0.5f - static_cast<f32>(cx);
            const f32 dy = static_cast<f32>(y) + 0.5f - static_cast<f32>(cy);
            const f32 d = std::sqrt(dx * dx + dy * dy);
            if (d <= r) {
                set_px(rgb, w, h, x, y, cr, cg, cb);
            } else if (d <= r + b) {
                set_px(rgb, w, h, x, y, 0u, 0u, 0u);
            }
        }
    }
}

static void score_rgb (i32 score, i32 lo, i32 hi, u8* r, u8* g, u8* b) {
    f32 t = 0.5f;
    if (hi > lo) {
        t = static_cast<f32>(score - lo) / static_cast<f32>(hi - lo);
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    *r = static_cast<u8>(255.0f * t + 0.5f);
    *g = 0u;
    *b = static_cast<u8>(255.0f * (1.0f - t) + 0.5f);
}

static bool save_map (
    cstr path,
    const GameArraySimple& map,
    const SpgCoordPair* starts,
    const i32* scores,
    u16 n)
{
    if (path == nullptr || starts == nullptr || scores == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u) {
        return false;
    }
    i32 lo = scores[0];
    i32 hi = scores[0];
    for (u16 i = 1; i < n; ++i) {
        if (scores[i] < lo) {
            lo = scores[i];
        }
        if (scores[i] > hi) {
            hi = scores[i];
        }
    }
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> rgb(static_cast<size_t>(tn) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0u;
            u8 g = 0u;
            u8 b = 0u;
            climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = G_RIV_R;
                g = G_RIV_G;
                b = G_RIV_B;
            }
            if (map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120u;
                g = 72u;
                b = 40u;
            }
            const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
            rgb[i + 0u] = r;
            rgb[i + 1u] = g;
            rgb[i + 2u] = b;
        }
    }
    for (u16 i = 0; i < n; ++i) {
        u8 r = 0u;
        u8 g = 0u;
        u8 b = 0u;
        score_rgb(scores[i], lo, hi, &r, &g, &b);
        draw_disk(rgb.data(), w, h, starts[i].x, starts[i].y, G_MARK_R, r, g, b, G_BORDER);
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(tn) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("*** FAILED path/out\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("*** FAILED load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    if (!TileYields::setup(st)) {
        std::printf("*** FAILED TileYields::setup\n");
        return 1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    (void)Factory_GameArraySimple::load_res_dist_data(&map, g_res);
    MapTerrainData terr;
    if (!MapLoader::load_terrain_ppm(g_terr, terr)) {
        std::printf("*** FAILED load terrain for starts\n");
        return 1;
    }
    const u16 mw = map.width();
    const u16 mh = map.height();
    u16 latt_r = 0u;
    u16 latt_c = 0u;
    latt_for_map(mw, mh, &latt_r, &latt_c);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.pick_n = G_PICK_N;
    par.latt_rows = latt_r;
    par.latt_cols = latt_c;
    par.seed = G_SEED;
    StartingPointGenerator spg(par);
    if (!spg.generate()) {
        std::printf("*** FAILED generate starts\n");
        return 1;
    }
    const SpgPickCoords starts = spg.picks_coords();
    if (starts.n == 0u) {
        std::printf("*** FAILED no starts\n");
        return 1;
    }
    std::vector<i32> scores(starts.n, 0);
    if (!LandPotential::score(map, starts.pts, static_cast<u16>(starts.n), scores.data())) {
        std::printf("*** FAILED LandPotential::score\n");
        return 1;
    }
    i32 lo = scores[0];
    i32 hi = scores[0];
    for (u32 i = 1; i < starts.n; ++i) {
        if (scores[i] < lo) {
            lo = scores[i];
        }
        if (scores[i] > hi) {
            hi = scores[i];
        }
    }
    char out_path[384];
    if (std::snprintf(out_path, sizeof(out_path), "%s/land_potential.ppm", G_OUT_DIR) <= 0) {
        return 1;
    }
    if (!save_map(out_path, map, starts.pts, scores.data(), static_cast<u16>(starts.n))) {
        std::printf("*** FAILED save %s\n", out_path);
        return 1;
    }
    std::printf("starts=%u score_min=%d score_max=%d box=%u\n",
        static_cast<unsigned>(starts.n), lo, hi, static_cast<unsigned>(LandPotential::k_box));
    std::printf("wrote %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
