//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>

#include "near_explore_mk1.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_terrain_validate.h"
#include "runtime_trace_dbg.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr NE_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr NE_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr NE_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr NE_OUT = "/home/w/Projects/simple-map-gen/near_explore_mk1_result.ppm";
static const cstr NE_FRAMES = "/home/w/Projects/simple-map-gen/near_explore_mk1_frames";
static const cstr NE_TRACE = "near_explore_mk1_test.trace";
static const u16 NE_SX = 499u;
static const u16 NE_SY = 499u;
static const u16 NE_TURNS = PATH_MP_TURN;
static const u16 NE_SIGHT = 3u;

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
    for (u32 d = 0u; d <= max_d; ++d) {
        for (u16 y = 0u; y < h; ++y) {
            for (u16 x = 0u; x < w; ++x) {
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

static bool ensure_dir (cstr path) {
    if (mkdir(path, 0755) == 0) {
        return true;
    }
    return errno == EEXIST;
}

static void mark_pt (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    if (px >= w || py >= h) {
        return;
    }
    const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void mark_cross (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    for (i16 dy = -2; dy <= 2; ++dy) {
        for (i16 dx = -2; dx <= 2; ++dx) {
            if ((dx < 0 ? -dx : dx) > 1 && (dy < 0 ? -dy : dy) > 1) {
                continue;
            }
            const i32 xi = static_cast<i32>(px) + dx;
            const i32 yi = static_cast<i32>(py) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            mark_pt(rgb, w, h, x, y, r, g, b);
        }
    }
}

static u32 cnt_exp (const u8* exp, u32 n) {
    u32 c = 0u;
    for (u32 i = 0u; i < n; ++i) {
        c += exp[i];
    }
    return c;
}

static bool save_frame_ppm (
    const GameArraySimple& map,
    const u8* exp,
    u16 ux,
    u16 uy,
    u16 hx,
    u16 hy,
    u8 ph,
    cstr path) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < n; ++i) {
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (exp[i] != 0u) {
            MapTerrainValidate::rgb_from_class(map.get_terrain(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = 0;
                g = 128;
                b = 255;
            }
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    mark_cross(rgb, w, h, hx, hy, 0, 255, 0);
    if (ph == 1u) {
        mark_pt(rgb, w, h, ux, uy, 0, 255, 255);
    } else {
        mark_pt(rgb, w, h, ux, uy, 255, 255, 0);
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

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    GameArraySimple map;
    if (!Factory_GameArraySimple::load(&map, NE_IN_TERR, NE_IN_CLIM, NE_IN_RIV)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    if (!ensure_dir(NE_FRAMES)) {
        std::printf("*** FAILED mkdir %s\n", NE_FRAMES);
        return 1;
    }
    TRACE_SETUP((NE_TRACE));
    u16 ux = NE_SX;
    u16 uy = NE_SY;
    if (!is_spawn_terr(map.get_terrain(ux, uy))) {
        if (!find_land_center(map, ux, uy)) {
            std::printf("*** FAILED find spawn\n");
            return 1;
        }
    }
    const u32 tile_n = map.tile_n();
    u8* exp = new u8[tile_n];
    if (exp == nullptr) {
        std::printf("*** FAILED alloc exp\n");
        return 1;
    }
    for (u32 i = 0u; i < tile_n; ++i) {
        exp[i] = 0u;
    }
    NearExploreMk1 ai(map, exp, ux, uy, NE_SIGHT, 0u, NE_BIAS_NONE);
    char path[160];
    u32 frame = 0u;
    std::snprintf(path, sizeof(path), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
    if (!save_frame_ppm(map, exp, ai.x(), ai.y(), ai.hx(), ai.hy(), ai.phase(), path)) {
        std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
        delete[] exp;
        return 1;
    }
    if (verbose) {
        std::printf("near_explore_mk1: %ux%u sight %u home (%u,%u)\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            static_cast<unsigned>(NE_SIGHT),
            static_cast<unsigned>(ai.hx()),
            static_cast<unsigned>(ai.hy()));
    }
    u16 turn = 0u;
    u32 steps = 0u;
    while (turn < NE_TURNS && !ai.done()) {
        ++turn;
        TRACE_NEW_TURN((turn));
        const u16 ox = ai.x();
        const u16 oy = ai.y();
        ai.move(1u);
        if (ai.x() == ox && ai.y() == oy) {
            break;
        }
        ++steps;
        ++frame;
        std::snprintf(path, sizeof(path), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
        if (!save_frame_ppm(map, exp, ai.x(), ai.y(), ai.hx(), ai.hy(), ai.phase(), path)) {
            std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
            delete[] exp;
            return 1;
        }
        if (verbose && (turn == 1u || (turn % 50u) == 0u)) {
            std::printf("  turn %u pos (%u,%u) phase %u explored %u riv %u\n",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(ai.x()),
                static_cast<unsigned>(ai.y()),
                static_cast<unsigned>(ai.phase()),
                static_cast<unsigned>(cnt_exp(exp, tile_n)),
                static_cast<unsigned>(ai.riv_spot() ? 1u : 0u));
        }
        if (ai.riv_spot()) {
            ai.clr_riv_spot();
        }
    }
    if (!save_frame_ppm(map, exp, ai.x(), ai.y(), ai.hx(), ai.hy(), ai.phase(), NE_OUT)) {
        std::printf("*** FAILED save %s\n", NE_OUT);
        delete[] exp;
        return 1;
    }
    std::printf("steps %u frames %u explored %u end (%u,%u) home (%u,%u) phase %u done %u loc_exh %u\n",
        static_cast<unsigned>(steps),
        static_cast<unsigned>(frame),
        static_cast<unsigned>(cnt_exp(exp, tile_n)),
        static_cast<unsigned>(ai.x()),
        static_cast<unsigned>(ai.y()),
        static_cast<unsigned>(ai.hx()),
        static_cast<unsigned>(ai.hy()),
        static_cast<unsigned>(ai.phase()),
        static_cast<unsigned>(ai.done() ? 1u : 0u),
        static_cast<unsigned>(ai.loc_exh() ? 1u : 0u));
    std::printf("frames: %s\n", NE_FRAMES);
    std::printf("result: %s\n", NE_OUT);
    if (verbose) {
        std::printf("trace: %s\n", NE_TRACE);
        std::printf("*** PASSED near_explore_mk1\n");
    }
    delete[] exp;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
