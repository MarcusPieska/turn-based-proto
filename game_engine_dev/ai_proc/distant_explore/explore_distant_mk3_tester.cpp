//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "explore_distant_mk3.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_terrain_validate.h"
#include "runtime_trace_dbg.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr ED3_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr ED3_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr ED3_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr ED3_OUT = "/home/w/Projects/simple-map-gen/explore_distant_mk3_path.ppm";
static const cstr ED3_TRACE = "explore_distant_test.trace";
static const u16 ED3_SX = 499u;
static const u16 ED3_SY = 499u;
static const u16 ED3_SIGHT = 3u;
static const u16 ED3_TURNS = PATH_MP_TURN;
static const u16 ED3_MOVES = 3u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_spawn_terr (u8 t) {
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

static u32 cheb_dist (u16 x, u16 y, u16 cx, u16 cy) {
    const u32 dx = (x > cx) ? static_cast<u32>(x - cx) : static_cast<u32>(cx - x);
    const u32 dy = (y > cy) ? static_cast<u32>(y - cy) : static_cast<u32>(cy - y);
    return (dx > dy) ? dx : dy;
}

static bool find_land_center (const GameArraySimple& map, u16& ox, u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 cx = w / 2u;
    const u16 cy = h / 2u;
    const u32 max_d = static_cast<u32>(cx > cy ? cx : cy) + 1u;
    for (u32 d = 0; d <= max_d; ++d) {
        for (u16 y = 0; y < h; ++y) {
            for (u16 x = 0; x < w; ++x) {
                if (cheb_dist(x, y, cx, cy) != d) {
                    continue;
                }
                if (!is_spawn_terr(map.get_terrain(x, y))) {
                    continue;
                }
                ox = x;
                oy = y;
                return true;
            }
        }
    }
    return false;
}

static u32 cnt_explored (const MapBitOverlay& ov) {
    u32 n = 0;
    for (u16 y = 0; y < ov.height(); ++y) {
        for (u16 x = 0; x < ov.width(); ++x) {
            n += ov.get(x, y);
        }
    }
    return n;
}

static bool save_wp_ppm (
    const GameArraySimple& map,
    const ExploreDistantMk3& ai,
    cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        MapTerrainValidate::rgb_from_class(map.get_terrain(x, y), &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u16 wi = 0; wi < ai.path_n(); ++wi) {
        const u16 wx = ai.wp_x(wi);
        const u16 wy = ai.wp_y(wi);
        const u32 i = static_cast<u32>(wy) * static_cast<u32>(w) + static_cast<u32>(wx);
        rgb[i * 3u + 0] = 255;
        rgb[i * 3u + 1] = 0;
        rgb[i * 3u + 2] = 0;
    }
    const u16 sx = ai.sx();
    const u16 sy = ai.sy();
    for (i16 dy = -2; dy <= 2; ++dy) {
        for (i16 dx = -2; dx <= 2; ++dx) {
            const i32 xi = static_cast<i32>(sx) + dx;
            const i32 yi = static_cast<i32>(sy) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h) {
                continue;
            }
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            rgb[i * 3u + 0] = 255;
            rgb[i * 3u + 1] = 255;
            rgb[i * 3u + 2] = 0;
        }
    }
    FILE* fp = std::fopen(path, "wb");
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
//=> - Main -
//================================================================================================================================

int main () {
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, ED3_IN_TERR, ED3_IN_CLIM, ED3_IN_RIV)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    TRACE_SETUP((ED3_TRACE));
    u16 ux = ED3_SX;
    u16 uy = ED3_SY;
    if (!is_spawn_terr(map.get_terrain(ux, uy))) {
        if (!find_land_center(map, ux, uy)) {
            std::printf("*** FAILED find land start\n");
            return 1;
        }
    }
    MapBitOverlay exp(map.width(), map.height());
    const clock_t t0 = clock();
    ExploreDistantMk3 ai(map, exp, ux, uy, ED3_SIGHT, 0u);
    const clock_t t1 = clock();
    const double derive_sec = ai.derive_sec();
    const double ctor_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!save_wp_ppm(map, ai, ED3_OUT)) {
        std::printf("*** FAILED save %s\n", ED3_OUT);
        return 1;
    }
    exp.set(ux, uy);
    TRACE_EXPLORE_DISCOVER((ux, uy, 0u));
    std::printf("explore_distant_mk3: %ux%u turns %u moves %u sight %u\n",
        static_cast<unsigned>(map.width()),
        static_cast<unsigned>(map.height()),
        static_cast<unsigned>(ED3_TURNS),
        static_cast<unsigned>(ED3_MOVES),
        static_cast<unsigned>(ED3_SIGHT));
    std::printf("derive_path time: %.6f s (ctor %.6f s)\n", derive_sec, ctor_sec);
    std::printf("start (%u,%u) waypoints %u\n",
        static_cast<unsigned>(ux),
        static_cast<unsigned>(uy),
        static_cast<unsigned>(ai.path_n()));
    std::printf("waypoint map: %s\n", ED3_OUT);
    for (u16 turn = 1; turn <= ED3_TURNS; ++turn) {
        TRACE_NEW_TURN((turn));
        ai.move(ED3_MOVES);
    }
    std::printf("unit end (%u,%u) waypoints %u/%u explored %u\n",
        static_cast<unsigned>(ai.x()),
        static_cast<unsigned>(ai.y()),
        static_cast<unsigned>(ai.wp_i()),
        static_cast<unsigned>(ai.path_n()),
        cnt_explored(exp));
    std::printf("trace: %s\n", ED3_TRACE);
    std::printf("*** PASSED explore_distant_mk3\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
