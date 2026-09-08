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

#include "continent_size_indexer.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "land_potential.h"
#include "location_grouper.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "runtime_static_loader.h"
#include "starting_point_generator.h"
#include "tile_yields.h"
#include "whiteboard_mng.h"

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

static bool save_cont (
    cstr path,
    u16 w,
    u16 h,
    u16 rank,
    const u16* land_idx,
    const u32* cont_rgb,
    const LocGroup& g)
{
    if (path == nullptr || land_idx == nullptr || cont_rgb == nullptr) {
        return false;
    }
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> rgb(static_cast<size_t>(tn) * 3u, 0u);
    for (u32 i = 0; i < tn; ++i) {
        if (land_idx[i] != rank) {
            continue;
        }
        const u32 v = cont_rgb[i];
        rgb[i * 3u + 0u] = static_cast<u8>((v >> 16) & 0xffu);
        rgb[i * 3u + 1u] = static_cast<u8>((v >> 8) & 0xffu);
        rgb[i * 3u + 2u] = static_cast<u8>(v & 0xffu);
    }
    if (g.m_n > 0u && g.m_pts != nullptr && g.m_sc != nullptr) {
        i32 lo = g.m_sc[0];
        i32 hi = g.m_sc[0];
        for (u16 i = 1; i < g.m_n; ++i) {
            if (g.m_sc[i] < lo) {
                lo = g.m_sc[i];
            }
            if (g.m_sc[i] > hi) {
                hi = g.m_sc[i];
            }
        }
        for (u16 i = 0; i < g.m_n; ++i) {
            u8 r = 0u;
            u8 gg = 0u;
            u8 b = 0u;
            score_rgb(g.m_sc[i], lo, hi, &r, &gg, &b);
            draw_disk(rgb.data(), w, h, g.m_pts[i].x, g.m_pts[i].y, G_MARK_R, r, gg, b, G_BORDER);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
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
        std::printf("*** FAILED load terrain\n");
        return 1;
    }
    const u16 mw = map.width();
    const u16 mh = map.height();
    const u8* cls = terr.data();
    if (cls == nullptr) {
        std::printf("*** FAILED empty terrain\n");
        return 1;
    }
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
    WhiteboardMng::init(mw, mh);
    Whiteboard_4B wb_rgb("location_grouper_tester", "rgb", 0u);
    Whiteboard_2B wb_idx("location_grouper_tester", "idx", 0u);
    if (!wb_rgb.ok() || !wb_idx.ok()) {
        std::printf("*** FAILED whiteboard\n");
        WhiteboardMng::terminate();
        return 1;
    }
    ContSizeList clist = {};
    if (!ContinentSizeIndexer::index(cls, mw, mh, &clist, wb_rgb, wb_idx)) {
        std::printf("*** FAILED ContinentSizeIndexer::index\n");
        WhiteboardMng::terminate();
        return 1;
    }
    LocGroups groups;
    if (!LocationGrouper::group(
            starts.pts,
            scores.data(),
            static_cast<u16>(starts.n),
            wb_idx.get_iter_ptr(),
            mw,
            mh,
            clist.m_n,
            &groups)) {
        std::printf("*** FAILED LocationGrouper::group\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 wrote = 0u;
    for (u16 i = 0; i < groups.m_n; ++i) {
        const LocGroup& g = groups.m_g[i];
        if (g.m_n == 0u) {
            continue;
        }
        char out_path[384];
        if (std::snprintf(out_path, sizeof(out_path), "%s/group_cont_%02u.ppm", G_OUT_DIR, static_cast<unsigned>(i + 1u)) <= 0) {
            WhiteboardMng::terminate();
            return 1;
        }
        if (!save_cont(out_path, mw, mh, g.m_rank, wb_idx.get_iter_ptr(), wb_rgb.get_iter_ptr(), g)) {
            std::printf("*** FAILED save %s\n", out_path);
            WhiteboardMng::terminate();
            return 1;
        }
        std::printf("cont=%u rank=%u locs=%u top_score=%d wrote %s\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(g.m_rank),
            static_cast<unsigned>(g.m_n),
            g.m_sc[0],
            out_path);
        ++wrote;
    }
    std::printf("starts=%u continents=%u images=%u\n",
        static_cast<unsigned>(starts.n),
        static_cast<unsigned>(groups.m_n),
        static_cast<unsigned>(wrote));
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
