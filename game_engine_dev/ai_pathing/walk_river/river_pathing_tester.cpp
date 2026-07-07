//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>

#include "river_pathing.h"
#include "factory_game_array_simple.h"
#include "map_bit_overlay.h"
#include "map_loader.h"
#include "path_mk1.h"

typedef const char* cstr;

static const cstr k_in_terr = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr k_in_clim = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr k_in_riv = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";

int main (int argc, char** argv) {
    (void)argc;
    (void)argv;
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, k_in_terr, k_in_clim, k_in_riv)) {
        std::printf("river_pathing: load failed\n");
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    MapBitOverlay exp(w, h);
    u16 sx = 0u;
    u16 sy = 0u;
    bool found = false;
    for (u16 y = 0u; y < h && !found; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (map.get_river(x, y) != 0u) {
                sx = x;
                sy = y;
                found = true;
                break;
            }
        }
    }
    if (!found) {
        std::printf("river_pathing: no river tile\n");
        return 1;
    }
    exp.set(sx, sy);
    const u16 sight = 3u;
    u16 ox = 0u;
    u16 oy = 0u;
    const bool pick = RiverPathing::pick_near_front(map, exp, sx, sy, sight, ox, oy, 0u, 0u, false);
    PathMk1 path;
    const bool have = RiverPathing::find_path_to_front(map, exp, sx, sy, sight, path);
    std::printf("river_pathing: spawn (%u,%u) pick=%d path_n=%u\n",
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        pick ? 1 : 0, static_cast<unsigned>(path.n()));
    if (!have || path.n() == 0u) {
        std::printf("*** FAILED river_pathing\n");
        return 1;
    }
    std::printf("*** PASSED river_pathing\n");
    return 0;
}
