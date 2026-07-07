//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "explore_distant_mk2.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* ED_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const char* ED_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const char* ED_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const char* ED_TRACE = "explore_distant_test.trace";

static const u16 ED_TURNS = PATH_MP_TURN;
static const u16 ED_MOVES = 3;
static const u16 ED_SIGHT = 3;

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

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, ED_IN_TERR, ED_IN_CLIM, ED_IN_RIV)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    TRACE_SETUP((ED_TRACE));
    MapBitOverlay exp(map.width(), map.height());
    u16 ux = 0;
    u16 uy = 0;
    if (!find_land_center(map, ux, uy)) {
        std::printf("*** FAILED find land start\n");
        return 1;
    }
    ExploreDistantMk2 ai(map, exp, ux, uy, ED_SIGHT, 0u);
    exp.set(ux, uy);
    TRACE_EXPLORE_DISCOVER((ux, uy, 0u));
    std::printf("explore_distant_mk2: %ux%u turns %u moves %u sight %u\n",
        static_cast<unsigned>(map.width()),
        static_cast<unsigned>(map.height()),
        static_cast<unsigned>(ED_TURNS),
        static_cast<unsigned>(ED_MOVES),
        static_cast<unsigned>(ED_SIGHT));
    std::printf("unit start (%u,%u)\n",
        static_cast<unsigned>(ux),
        static_cast<unsigned>(uy));
    for (u16 turn = 1; turn <= ED_TURNS; ++turn) {
        TRACE_NEW_TURN((turn));
        ai.move(ED_MOVES);
    }
    std::printf("unit end (%u,%u) explored %u\n",
        static_cast<unsigned>(ai.x()),
        static_cast<unsigned>(ai.y()),
        cnt_explored(exp));
    std::printf("trace: %s\n", ED_TRACE);
    std::printf("*** PASSED explore_distant\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
