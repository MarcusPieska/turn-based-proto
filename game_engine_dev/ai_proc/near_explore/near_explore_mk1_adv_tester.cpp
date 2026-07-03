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

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr NE_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr NE_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr NE_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr NE_OUT = "/home/w/Projects/simple-map-gen/near_explore_mk1_adv_result.ppm";
static const cstr NE_FRAMES = "/home/w/Projects/simple-map-gen/near_explore_mk1_adv_frames";
static const u16 NE_SX = 499u;
static const u16 NE_SY = 499u;
static const u16 NE_TURNS = PATH_MP_TURN;
static const u16 NE_SIGHT = 3u;
static const u32 NE_UNIT_N = 4u;

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

static bool save_adv_ppm (
    const GameArraySimple& map,
    const u8* exp,
    NearExploreMk1* const* ai,
    const u8* ur,
    const u8* ug,
    const u8* ub,
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
    mark_cross(rgb, w, h, ai[0]->hx(), ai[0]->hy(), 255, 255, 255);
    for (u32 ui = 0u; ui < NE_UNIT_N; ++ui) {
        mark_pt(rgb, w, h, ai[ui]->x(), ai[ui]->y(), ur[ui], ug[ui], ub[ui]);
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

static const char* bias_name (NearExploreBias b) {
    if (b == NE_BIAS_NORTH) {
        return "north";
    }
    if (b == NE_BIAS_SOUTH) {
        return "south";
    }
    if (b == NE_BIAS_EAST) {
        return "east";
    }
    if (b == NE_BIAS_WEST) {
        return "west";
    }
    return "none";
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
    const NearExploreBias bias[NE_UNIT_N] = {
        NE_BIAS_NORTH,
        NE_BIAS_SOUTH,
        NE_BIAS_EAST,
        NE_BIAS_WEST
    };
    const u8 ur[NE_UNIT_N] = {255, 0, 0, 255};
    const u8 ug[NE_UNIT_N] = {0, 255, 255, 0};
    const u8 ub[NE_UNIT_N] = {0, 0, 255, 255};
    NearExploreMk1 ai_n(map, exp, ux, uy, NE_SIGHT, 0u, NE_BIAS_NORTH);
    NearExploreMk1 ai_s(map, exp, ux, uy, NE_SIGHT, 1u, NE_BIAS_SOUTH);
    NearExploreMk1 ai_e(map, exp, ux, uy, NE_SIGHT, 2u, NE_BIAS_EAST);
    NearExploreMk1 ai_w(map, exp, ux, uy, NE_SIGHT, 3u, NE_BIAS_WEST);
    NearExploreMk1* ai[NE_UNIT_N] = {&ai_n, &ai_s, &ai_e, &ai_w};
    char path[160];
    u32 frame = 0u;
    std::snprintf(path, sizeof(path), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
    if (!save_adv_ppm(map, exp, ai, ur, ug, ub, path)) {
        std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
        delete[] exp;
        return 1;
    }
    if (verbose) {
        std::printf("near_explore_mk1_adv: %ux%u sight %u home (%u,%u)\n",
            static_cast<unsigned>(map.width()),
            static_cast<unsigned>(map.height()),
            static_cast<unsigned>(NE_SIGHT),
            static_cast<unsigned>(ux),
            static_cast<unsigned>(uy));
    }
    u16 turn = 0u;
    u32 steps[NE_UNIT_N] = {0u, 0u, 0u, 0u};
    while (turn < NE_TURNS) {
        bool any = false;
        for (u32 ui = 0u; ui < NE_UNIT_N; ++ui) {
            if (!ai[ui]->done()) {
                any = true;
                break;
            }
        }
        if (!any) {
            break;
        }
        ++turn;
        for (u32 ui = 0u; ui < NE_UNIT_N; ++ui) {
            if (ai[ui]->done()) {
                continue;
            }
            const u16 ox = ai[ui]->x();
            const u16 oy = ai[ui]->y();
            ai[ui]->move(1u);
            if (ai[ui]->x() != ox || ai[ui]->y() != oy) {
                ++steps[ui];
            }
            if (ai[ui]->riv_spot()) {
                ai[ui]->clr_riv_spot();
            }
        }
        ++frame;
        std::snprintf(path, sizeof(path), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
        if (!save_adv_ppm(map, exp, ai, ur, ug, ub, path)) {
            std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
            delete[] exp;
            return 1;
        }
        if (verbose && (turn == 1u || (turn % 100u) == 0u)) {
            std::printf("  turn %u explored %u\n",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(cnt_exp(exp, tile_n)));
            for (u32 ui = 0u; ui < NE_UNIT_N; ++ui) {
                std::printf("    %s (%u,%u) steps %u done %u\n",
                    bias_name(bias[ui]),
                    static_cast<unsigned>(ai[ui]->x()),
                    static_cast<unsigned>(ai[ui]->y()),
                    static_cast<unsigned>(steps[ui]),
                    static_cast<unsigned>(ai[ui]->done() ? 1u : 0u));
            }
        }
    }
    if (!save_adv_ppm(map, exp, ai, ur, ug, ub, NE_OUT)) {
        std::printf("*** FAILED save %s\n", NE_OUT);
        delete[] exp;
        return 1;
    }
    std::printf("turns %u frames %u explored %u\n",
        static_cast<unsigned>(turn),
        static_cast<unsigned>(frame),
        static_cast<unsigned>(cnt_exp(exp, tile_n)));
    for (u32 ui = 0u; ui < NE_UNIT_N; ++ui) {
        std::printf("  %s end (%u,%u) steps %u done %u loc_exh %u\n",
            bias_name(bias[ui]),
            static_cast<unsigned>(ai[ui]->x()),
            static_cast<unsigned>(ai[ui]->y()),
            static_cast<unsigned>(steps[ui]),
            static_cast<unsigned>(ai[ui]->done() ? 1u : 0u),
            static_cast<unsigned>(ai[ui]->loc_exh() ? 1u : 0u));
    }
    std::printf("frames: %s\n", NE_FRAMES);
    std::printf("result: %s\n", NE_OUT);
    if (verbose) {
        std::printf("*** PASSED near_explore_mk1_adv\n");
    }
    delete[] exp;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
