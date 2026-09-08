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
#include "lucky_seat_loader.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_LUCKY_LIB = "../../game/lucky_seats/lucky_seats.so";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/lucky-seat";
static const u32 G_SEED = 101u;
static const u16 G_PICK_N = 100u;
static const u16 G_LATT_DIV = 10u;
static const u16 G_NORM_R = 5u;
static const u16 G_LUCKY_R = 7u;
static const u16 G_BORDER = 2u;
static const u16 G_CROP = 100u;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;
static const u8 G_RES_A = 140u;
static const u16 G_LUCKY_PCT = 10u;

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
    *r = 0u;
    *g = 0u;
    *b = 0u;
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

static bool is_lucky (const u16* seats, u16 n, u16 seat) {
    for (u16 i = 0; i < n; ++i) {
        if (seats[i] == seat) {
            return true;
        }
    }
    return false;
}

static void count_res (const GameArraySimple& map, std::vector<u32>& cnt) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 r = map.get_res(x, y);
            if (r == U16_KEY_NULL || r >= cnt.size()) {
                continue;
            }
            ++cnt[r];
        }
    }
}

static u8 blend_u8 (u8 base, u8 over, u8 a) {
    const u16 inv = static_cast<u16>(255u - a);
    return static_cast<u8>((static_cast<u16>(base) * inv + static_cast<u16>(over) * a) / 255u);
}

static bool save_crop (
    cstr path,
    const GameArraySimple& map,
    u16 cx,
    u16 cy)
{
    if (path == nullptr) {
        return false;
    }
    const u16 mw = map.width();
    const u16 mh = map.height();
    const i32 half = static_cast<i32>(G_CROP / 2u);
    const i32 x0 = static_cast<i32>(cx) - half;
    const i32 y0 = static_cast<i32>(cy) - half;
    std::vector<u8> rgb(static_cast<size_t>(G_CROP) * static_cast<size_t>(G_CROP) * 3u, 0u);
    for (u16 ly = 0; ly < G_CROP; ++ly) {
        for (u16 lx = 0; lx < G_CROP; ++lx) {
            const i32 mx = x0 + static_cast<i32>(lx);
            const i32 my = y0 + static_cast<i32>(ly);
            const u32 pi = (static_cast<u32>(ly) * static_cast<u32>(G_CROP) + static_cast<u32>(lx)) * 3u;
            if (mx < 0 || my < 0 || mx >= static_cast<i32>(mw) || my >= static_cast<i32>(mh)) {
                continue;
            }
            const u16 x = static_cast<u16>(mx);
            const u16 y = static_cast<u16>(my);
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
            if (map.get_res(x, y) != U16_KEY_NULL) {
                r = blend_u8(r, 0u, G_RES_A);
                g = blend_u8(g, 0u, G_RES_A);
                b = blend_u8(b, 0u, G_RES_A);
            }
            rgb[pi + 0u] = r;
            rgb[pi + 1u] = g;
            rgb[pi + 2u] = b;
        }
    }
    {
        const u16 lx = static_cast<u16>(G_CROP / 2u);
        const u16 ly = static_cast<u16>(G_CROP / 2u);
        const u32 pi = (static_cast<u32>(ly) * static_cast<u32>(G_CROP) + static_cast<u32>(lx)) * 3u;
        rgb[pi + 0u] = 220u;
        rgb[pi + 1u] = 30u;
        rgb[pi + 2u] = 30u;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(G_CROP), static_cast<unsigned>(G_CROP));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
    std::fclose(fp);
    return ok;
}

static bool save_map (
    cstr path,
    const MapTerrainData& terr,
    const SpgPickCoords& starts,
    const u16* lucky_seats,
    u16 lucky_n)
{
    if (path == nullptr) {
        return false;
    }
    const u16 w = terr.width();
    const u16 h = terr.height();
    const u8* cls = terr.data();
    if (cls == nullptr || w == 0u || h == 0u) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> rgb(static_cast<size_t>(n) * 3u);
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0u;
        u8 g = 0u;
        u8 b = 0u;
        terr_rgb(cls[i], &r, &g, &b);
        rgb[i * 3u + 0u] = r;
        rgb[i * 3u + 1u] = g;
        rgb[i * 3u + 2u] = b;
    }
    for (u32 i = 0; i < starts.n; ++i) {
        const u16 sx = starts.pts[i].x;
        const u16 sy = starts.pts[i].y;
        if (is_lucky(lucky_seats, lucky_n, static_cast<u16>(i))) {
            draw_disk(rgb.data(), w, h, sx, sy, G_LUCKY_R, 220u, 40u, 40u, G_BORDER);
        } else {
            draw_disk(rgb.data(), w, h, sx, sy, G_NORM_R, 40u, 90u, 220u, G_BORDER);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
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
    LuckySeatLoader lucky_ldr;
    if (!lucky_ldr.load(G_LUCKY_LIB)) {
        std::printf("*** FAILED load %s\n", G_LUCKY_LIB);
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
    u16 latt_r = 0u;
    u16 latt_c = 0u;
    latt_for_map(terr.width(), terr.height(), &latt_r, &latt_c);
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
    u16 lucky_seats[SPG_MAX_PICK_PTS];
    LuckySeatReq req = {};
    req.m_map = &map;
    req.m_statics = &st;
    req.m_starts = starts.pts;
    req.m_start_n = static_cast<u16>(starts.n);
    req.m_lucky_seats = lucky_seats;
    req.m_lucky_cap = SPG_MAX_PICK_PTS;
    req.m_lucky_n = 0u;
    req.m_do_select = 1u;
    req.m_do_boost = 0u;
    LuckySeatRslt sel = lucky_ldr.run(&req);
    if (!sel.m_ok) {
        std::printf("*** FAILED lucky_seat select\n");
        return 1;
    }
    const u16 res_n = st.resource().get_item_count();
    std::vector<u32> before(res_n, 0u);
    std::vector<u32> after(res_n, 0u);
    count_res(map, before);
    for (u16 i = 0; i < req.m_lucky_n; ++i) {
        const u16 s = lucky_seats[i];
        char crop_a[384];
        if (std::snprintf(crop_a, sizeof(crop_a), "%s/lucky_res_%02ua.ppm", G_OUT_DIR, static_cast<unsigned>(i + 1u)) <= 0) {
            return 1;
        }
        if (!save_crop(crop_a, map, starts.pts[s].x, starts.pts[s].y)) {
            std::printf("*** FAILED save %s\n", crop_a);
            return 1;
        }
        std::printf("wrote %s\n", crop_a);
    }
    req.m_do_select = 0u;
    req.m_do_boost = 1u;
    LuckySeatRslt boost = lucky_ldr.run(&req);
    if (!boost.m_ok) {
        std::printf("*** FAILED lucky_seat boost\n");
        return 1;
    }
    count_res(map, after);
    const u32 added = boost.m_local_n + boost.m_river_n;
    u32 delta_sum = 0u;
    std::printf("resource_boost local=%u river=%u total=%u\n",
        static_cast<unsigned>(boost.m_local_n),
        static_cast<unsigned>(boost.m_river_n),
        static_cast<unsigned>(added));
    for (u16 i = 0; i < res_n; ++i) {
        if (after[i] <= before[i]) {
            continue;
        }
        const u32 d = after[i] - before[i];
        delta_sum += d;
        const char* nm = st.resource().get_name(ResourceStaticDataKey::from_raw(i));
        std::printf("  +%u %s (before=%u after=%u)\n",
            static_cast<unsigned>(d),
            nm != nullptr ? nm : "?",
            static_cast<unsigned>(before[i]),
            static_cast<unsigned>(after[i]));
    }
    std::printf("resource_boost delta_sum=%u\n", static_cast<unsigned>(delta_sum));
    for (u16 i = 0; i < req.m_lucky_n; ++i) {
        const u16 s = lucky_seats[i];
        char crop_c[384];
        if (std::snprintf(crop_c, sizeof(crop_c), "%s/lucky_res_%02uc.ppm", G_OUT_DIR, static_cast<unsigned>(i + 1u)) <= 0) {
            return 1;
        }
        if (!save_crop(crop_c, map, starts.pts[s].x, starts.pts[s].y)) {
            std::printf("*** FAILED save %s\n", crop_c);
            return 1;
        }
        std::printf("wrote %s\n", crop_c);
    }
    char out_path[384];
    if (std::snprintf(out_path, sizeof(out_path), "%s/lucky_seats.ppm", G_OUT_DIR) <= 0) {
        return 1;
    }
    if (!save_map(out_path, terr, starts, lucky_seats, req.m_lucky_n)) {
        std::printf("*** FAILED save %s\n", out_path);
        return 1;
    }
    const u16 target = static_cast<u16>((starts.n * G_LUCKY_PCT) / 100u);
    std::printf("starts=%u lucky=%u target=%u\n",
        static_cast<unsigned>(starts.n),
        static_cast<unsigned>(req.m_lucky_n),
        static_cast<unsigned>(target));
    for (u16 i = 0; i < req.m_lucky_n; ++i) {
        const u16 s = lucky_seats[i];
        std::printf("  lucky seat=%u at (%u,%u)\n",
            static_cast<unsigned>(s),
            static_cast<unsigned>(starts.pts[s].x),
            static_cast<unsigned>(starts.pts[s].y));
    }
    std::printf("wrote %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
