//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstdarg>

#include "near_path_mk1.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_loader.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr NP_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr NP_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr NP_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";

static const cstr k_ansi_rst = "\033[0m";
static const cstr k_ansi_grn = "\033[32m";
static const cstr k_ansi_red = "\033[31m";
static const cstr k_ansi_blu = "\033[94m";

static void print_cls_size (size_t n) {
    if (n < 1000u) {
        std::printf("%sTotal size: %zuB%s\n", k_ansi_blu, n, k_ansi_rst);
    } else {
        std::printf("%sTotal size: %.2fKB%s\n", k_ansi_blu, static_cast<double>(n) / 1024.0, k_ansi_rst);
    }
}

static void t_fail (cstr fmt, ...) {
    va_list ap;
    std::printf("%s", k_ansi_red);
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("%s", k_ansi_rst);
}

static void t_pass (cstr fmt, ...) {
    va_list ap;
    std::printf("%s", k_ansi_grn);
    va_start(ap, fmt);
    std::vprintf(fmt, ap);
    va_end(ap);
    std::printf("%s", k_ansi_rst);
}

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_walk (u8 t) {
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    print_cls_size(sizeof(NearPathMk1));
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, NP_IN_TERR, NP_IN_CLIM, NP_IN_RIV)) {
        t_fail("*** FAILED load map\n");
        return 1;
    }
    NearPathMk1 path(map);
    const u16 w = map.width();
    const u16 h = map.height();
    u16 sx = 0u;
    u16 sy = 0u;
    u16 gx = 0u;
    u16 gy = 0u;
    bool have = false;
    for (u16 y = 0u; y < h && !have; ++y) {
         for (u16 x = 0u; x < w; ++x) {
            if (!is_walk(map.get_terrain(x, y))) {
                continue;
            }
            sx = x;
            sy = y;
            for (u16 y2 = y; y2 < h && !have; ++y2) {
                for (u16 x2 = (y2 == y) ? static_cast<u16>(x + 1u) : 0u; x2 < w; ++x2) {
                    if (!is_walk(map.get_terrain(x2, y2))) {
                        continue;
                    }
                    if (path.can_reach(sx, sy, x2, y2)) {
                        gx = x2;
                        gy = y2;
                        have = true;
                        break;
                    }
                }
            }
            if (have) {
                break;
            }
        }
    }
    if (!have) {
        t_fail("*** FAILED find reachable pair\n");
        return 1;
    }
    const u32 cheb = NearPathMk1::cheb(sx, sy, gx, gy);
    u16 ox = 0u;
    u16 oy = 0u;
    const bool step = path.one_step(sx, sy, gx, gy, ox, oy);
    if (verbose) {
        std::printf("near_path_mk1: cheb=%u step=%d (%u,%u)->(%u,%u)\n",
            cheb, step ? 1 : 0,
            static_cast<unsigned>(sx), static_cast<unsigned>(sy),
            static_cast<unsigned>(ox), static_cast<unsigned>(oy));
    }
    if (!step) {
        t_fail("*** FAILED one_step\n");
        return 1;
    }
    if (verbose) {
        t_pass("*** PASSED near_path_mk1\n");
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
