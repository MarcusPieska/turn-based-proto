//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <ctime>

#include <sys/stat.h>
#include <sys/types.h>

#include "near_explore_mk1.h"
#include "river_explore_mk2.h"
#include "explore_distant_mk3.h"
#include "short_range_pathing.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_bit_overlay.h"
#include "map_terrain_validate.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr NE_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr NE_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr NE_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr NE_OUT = "/home/w/Projects/simple-map-gen/near_explore_mk1_hybrid_result.ppm";
static const cstr NE_FRAMES = "/home/w/Projects/simple-map-gen/near_explore_mk1_hybrid_frames";
static const u16 NE_SX = 499u;
static const u16 NE_SY = 499u;
static const u16 NE_TURNS = PATH_MP_TURN;
static const u16 NE_SIGHT = 3u;
static const u16 NE_SPAWN_GAP = 15u;
static const u16 NE_RIV_MOVES = 4u;
static const u8 NE_RIV_DONE = 3u;
static const u32 NE_UNIT_MAX = 4u;
static const u8 k_st_wait = 0u;
static const u8 k_st_near = 1u;
static const u8 k_st_path = 2u;
static const u8 k_st_river = 3u;
static const u8 k_st_distant = 4u;

//================================================================================================================================
//=> - UnitSlot -
//================================================================================================================================

struct UnitSlot {
    NearExploreMk1* near;
    RiverExploreMk2* river;
    ExploreDistantMk3* distant;
    u8 st;
    u16 px;
    u16 py;
    u16 rtx;
    u16 rty;
    u16 spawn_t;
    NearExploreBias bias;
    u8 player;
    u32 near_steps;
    u32 path_steps;
    u32 riv_steps;
    u32 dist_steps;
    u32 riv_done;
};

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

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static void sync_exp_ov (const u8* exp, MapBitOverlay& ov, u16 w, u16 h) {
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (exp[tidx(w, x, y)] != 0u) {
                ov.set(x, y);
            }
        }
    }
}

static void sync_ov_exp (u8* exp, const MapBitOverlay& ov, u16 w, u16 h) {
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (ov.get(x, y) != 0u) {
                exp[tidx(w, x, y)] = 1u;
            }
        }
    }
}

static void reveal_pt (u8* exp, MapBitOverlay& ov, u16 w, u16 x, u16 y) {
    const u32 i = tidx(w, x, y);
    if (exp[i] != 0u) {
        return;
    }
    exp[i] = 1u;
    ov.set(x, y);
}

static void reveal_around (
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 x,
    u16 y,
    u16 sight) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(x) + dx;
            const i32 yi = static_cast<i32>(y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(xi);
            const u16 ay = static_cast<u16>(yi);
            if (ax >= w || ay >= h) {
                continue;
            }
            reveal_pt(exp, ov, w, ax, ay);
        }
    }
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
            mark_pt(rgb, w, h, static_cast<u16>(xi), static_cast<u16>(yi), r, g, b);
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

static void unit_pos (const UnitSlot& u, u16& x, u16& y) {
    if (u.st == k_st_near && u.near != nullptr) {
        x = u.near->x();
        y = u.near->y();
        return;
    }
    if (u.st == k_st_distant && u.distant != nullptr) {
        x = u.distant->x();
        y = u.distant->y();
        return;
    }
    if (u.st == k_st_river && u.river != nullptr) {
        x = u.river->x();
        y = u.river->y();
        return;
    }
    x = u.px;
    y = u.py;
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

static const char* st_name (u8 st) {
    if (st == k_st_near) {
        return "near";
    }
    if (st == k_st_path) {
        return "path";
    }
    if (st == k_st_river) {
        return "river";
    }
    if (st == k_st_distant) {
        return "distant";
    }
    return "wait";
}

static u32 parse_unit_n (int argc, char** argv) {
    u32 n = NE_UNIT_MAX;
    if (argc >= 3 && argv[2] != nullptr) {
        n = static_cast<u32>(std::atoi(argv[2]));
    }
    if (n < 1u) {
        n = 1u;
    }
    if (n > NE_UNIT_MAX) {
        n = NE_UNIT_MAX;
    }
    return n;
}

static bool save_hyb_ppm (
    const GameArraySimple& map,
    const u8* exp,
    const UnitSlot* slot,
    u32 unit_n,
    u16 hx,
    u16 hy,
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
    mark_cross(rgb, w, h, hx, hy, 255, 255, 255);
    for (u32 ui = 0u; ui < unit_n; ++ui) {
        if (slot[ui].st == k_st_wait) {
            continue;
        }
        u16 ux = 0;
        u16 uy = 0;
        unit_pos(slot[ui], ux, uy);
        mark_pt(rgb, w, h, ux, uy, ur[ui], ug[ui], ub[ui]);
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

static bool has_near_fog (
    const GameArraySimple& map,
    const u8* exp,
    u16 x,
    u16 y,
    u16 sight,
    u16 w) {
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(x) + dx;
            const i32 yi = static_cast<i32>(y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(xi);
            const u16 ay = static_cast<u16>(yi);
            if (ax >= map.width() || ay >= map.height()) {
                continue;
            }
            if (!is_spawn_terr(map.get_terrain(ax, ay))) {
                continue;
            }
            if (exp[tidx(w, ax, ay)] == 0u) {
                return true;
            }
        }
    }
    return false;
}

static void spawn_near (
    UnitSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sx,
    u16 sy,
    u16 sight) {
    u.near = new NearExploreMk1(map, exp, sx, sy, sight, u.player, u.bias);
    sync_exp_ov(exp, ov, map.width(), map.height());
    u.px = sx;
    u.py = sy;
    u.st = k_st_near;
}

static void spawn_distant (
    UnitSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sx,
    u16 sy,
    u16 sight) {
    sync_exp_ov(exp, ov, map.width(), map.height());
    u.distant = new ExploreDistantMk3(map, ov, sx, sy, sight, u.player);
    sync_ov_exp(exp, ov, map.width(), map.height());
    u.px = sx;
    u.py = sy;
    u.st = k_st_distant;
}

static void resume_explore (
    UnitSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sight) {
    if (has_near_fog(map, exp, u.px, u.py, sight, map.width())) {
        spawn_near(u, map, exp, ov, u.px, u.py, sight);
    } else {
        spawn_distant(u, map, exp, ov, u.px, u.py, sight);
    }
}

static void start_river (
    UnitSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sight) {
    sync_exp_ov(exp, ov, map.width(), map.height());
    u.river = new RiverExploreMk2(map, ov, u.px, u.py, sight, u.player);
    sync_ov_exp(exp, ov, map.width(), map.height());
    u.st = k_st_river;
}

static void tick_unit (
    UnitSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    const ShortRangePathing& path,
    u16 sight) {
    if (u.st == k_st_near) {
        if (u.near == nullptr) {
            return;
        }
        if (u.near->riv_spot()) {
            u.rtx = u.near->riv_x();
            u.rty = u.near->riv_y();
            u.near->clr_riv_spot();
            u.px = u.near->x();
            u.py = u.near->y();
            delete u.near;
            u.near = nullptr;
            u.st = k_st_path;
            return;
        }
        if (u.near->done()) {
            return;
        }
        u.near->move(1u);
        u.px = u.near->x();
        u.py = u.near->y();
        ++u.near_steps;
        sync_exp_ov(exp, ov, map.width(), map.height());
        return;
    }
    if (u.st == k_st_path) {
        if (u.px == u.rtx && u.py == u.rty) {
            start_river(u, map, exp, ov, sight);
            return;
        }
        if (map.get_river(u.px, u.py) != 0u) {
            start_river(u, map, exp, ov, sight);
            return;
        }
        u16 nx = u.px;
        u16 ny = u.py;
        if (!path.one_step(u.px, u.py, u.rtx, u.rty, nx, ny)) {
            resume_explore(u, map, exp, ov, sight);
            return;
        }
        u.px = nx;
        u.py = ny;
        reveal_around(map, exp, ov, u.px, u.py, sight);
        ++u.path_steps;
        return;
    }
    if (u.st == k_st_river) {
        if (u.river == nullptr) {
            return;
        }
        for (u16 m = 0u; m < NE_RIV_MOVES; ++m) {
            if (u.river->phase() == NE_RIV_DONE) {
                break;
            }
            u.river->move(1u);
            ++u.riv_steps;
        }
        sync_ov_exp(exp, ov, map.width(), map.height());
        u.px = u.river->x();
        u.py = u.river->y();
        if (u.river->phase() == NE_RIV_DONE) {
            ++u.riv_done;
            delete u.river;
            u.river = nullptr;
            resume_explore(u, map, exp, ov, sight);
        }
        return;
    }
    if (u.st == k_st_distant) {
        if (u.distant == nullptr) {
            return;
        }
        if (u.distant->riv_spot()) {
            u.rtx = u.distant->riv_x();
            u.rty = u.distant->riv_y();
            u.distant->clr_riv_spot();
            u.px = u.distant->x();
            u.py = u.distant->y();
            delete u.distant;
            u.distant = nullptr;
            u.st = k_st_path;
            return;
        }
        if (u.distant->done()) {
            u.px = u.distant->x();
            u.py = u.distant->y();
            delete u.distant;
            u.distant = nullptr;
            resume_explore(u, map, exp, ov, sight);
            return;
        }
        u.distant->move(1u);
        u.px = u.distant->x();
        u.py = u.distant->y();
        ++u.dist_steps;
        sync_ov_exp(exp, ov, map.width(), map.height());
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    const u32 unit_n = parse_unit_n(argc, argv);
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
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tile_n = map.tile_n();
    u8* exp = new u8[tile_n];
    if (exp == nullptr) {
        std::printf("*** FAILED alloc exp\n");
        return 1;
    }
    for (u32 i = 0u; i < tile_n; ++i) {
        exp[i] = 0u;
    }
    MapBitOverlay ov(w, h);
    ShortRangePathing path(map);
    const NearExploreBias bias[NE_UNIT_MAX] = {
        NE_BIAS_NORTH,
        NE_BIAS_SOUTH,
        NE_BIAS_EAST,
        NE_BIAS_WEST
    };
    const u8 ur[NE_UNIT_MAX] = {255, 0, 0, 255};
    const u8 ug[NE_UNIT_MAX] = {0, 255, 255, 0};
    const u8 ub[NE_UNIT_MAX] = {0, 0, 255, 255};
    UnitSlot slot[NE_UNIT_MAX];
    for (u32 ui = 0u; ui < unit_n; ++ui) {
        slot[ui].near = nullptr;
        slot[ui].river = nullptr;
        slot[ui].distant = nullptr;
        slot[ui].st = k_st_wait;
        slot[ui].px = ux;
        slot[ui].py = uy;
        slot[ui].rtx = 0u;
        slot[ui].rty = 0u;
        slot[ui].spawn_t = static_cast<u16>(ui * NE_SPAWN_GAP);
        slot[ui].bias = bias[ui];
        slot[ui].player = static_cast<u8>(ui);
        slot[ui].near_steps = 0u;
        slot[ui].path_steps = 0u;
        slot[ui].riv_steps = 0u;
        slot[ui].dist_steps = 0u;
        slot[ui].riv_done = 0u;
    }
    char fpath[160];
    u32 frame = 0u;
    std::snprintf(fpath, sizeof(fpath), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
    if (!save_hyb_ppm(map, exp, slot, unit_n, ux, uy, ur, ug, ub, fpath)) {
        std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
        delete[] exp;
        return 1;
    }
    if (verbose) {
        std::printf("near_explore_mk1_hybrid: %ux%u units %u sight %u home (%u,%u) spawn_gap %u riv_moves %u\n",
            static_cast<unsigned>(w),
            static_cast<unsigned>(h),
            static_cast<unsigned>(unit_n),
            static_cast<unsigned>(NE_SIGHT),
            static_cast<unsigned>(ux),
            static_cast<unsigned>(uy),
            static_cast<unsigned>(NE_SPAWN_GAP),
            static_cast<unsigned>(NE_RIV_MOVES));
    }
    const clock_t t0 = std::clock(); 
    u16 turn = 0u;
    while (turn < NE_TURNS) {
        for (u32 ui = 0u; ui < unit_n; ++ui) {
            UnitSlot& u = slot[ui];
            if (turn < u.spawn_t) {
                continue;
            }
            if (u.st == k_st_wait) {
                spawn_near(u, map, exp, ov, ux, uy, NE_SIGHT);
            }
            tick_unit(u, map, exp, ov, path, NE_SIGHT);
        }
        ++turn;
        ++frame;
        std::snprintf(fpath, sizeof(fpath), "%s/%05u.ppm", NE_FRAMES, static_cast<unsigned>(frame));
        if (!save_hyb_ppm(map, exp, slot, unit_n, ux, uy, ur, ug, ub, fpath)) {
            std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(frame));
            for (u32 ui = 0u; ui < unit_n; ++ui) {
                delete slot[ui].near;
                delete slot[ui].river;
                delete slot[ui].distant;
            }
            delete[] exp;
            return 1;
        }
        if (verbose && (turn == 1u || (turn % 100u) == 0u)) {
            std::printf("  turn %u explored %u\n",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(cnt_exp(exp, tile_n)));
            for (u32 ui = 0u; ui < unit_n; ++ui) {
                u16 px = 0;
                u16 py = 0;
                unit_pos(slot[ui], px, py);
                std::printf("    %s %s (%u,%u) near %u path %u riv %u dist %u done %u\n",
                    bias_name(slot[ui].bias),
                    st_name(slot[ui].st),
                    static_cast<unsigned>(px),
                    static_cast<unsigned>(py),
                    static_cast<unsigned>(slot[ui].near_steps),
                    static_cast<unsigned>(slot[ui].path_steps),
                    static_cast<unsigned>(slot[ui].riv_steps),
                    static_cast<unsigned>(slot[ui].dist_steps),
                    static_cast<unsigned>(slot[ui].riv_done));
            }
        }
    }
    const clock_t t1 = std::clock();
    if (!save_hyb_ppm(map, exp, slot, unit_n, ux, uy, ur, ug, ub, NE_OUT)) {
        std::printf("*** FAILED save %s\n", NE_OUT);
        for (u32 ui = 0u; ui < unit_n; ++ui) {
            delete slot[ui].near;
            delete slot[ui].river;
            delete slot[ui].distant;
        }
        delete[] exp;
        return 1;
    }
    const double run_sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    u32 tot_riv = 0u;
    for (u32 ui = 0u; ui < unit_n; ++ui) {
        tot_riv += slot[ui].riv_done;
    }
    std::printf("turns %u frames %u explored %u rivers %u units %u time %.3f s\n",
        static_cast<unsigned>(turn),
        static_cast<unsigned>(frame),
        static_cast<unsigned>(cnt_exp(exp, tile_n)),
        static_cast<unsigned>(tot_riv),
        static_cast<unsigned>(unit_n),
        run_sec);
    for (u32 ui = 0u; ui < unit_n; ++ui) {
        u16 px = 0;
        u16 py = 0;
        unit_pos(slot[ui], px, py);
        std::printf("  %s %s (%u,%u) near %u path %u riv %u dist %u rivers %u\n",
            bias_name(slot[ui].bias),
            st_name(slot[ui].st),
            static_cast<unsigned>(px),
            static_cast<unsigned>(py),
            static_cast<unsigned>(slot[ui].near_steps),
            static_cast<unsigned>(slot[ui].path_steps),
            static_cast<unsigned>(slot[ui].riv_steps),
            static_cast<unsigned>(slot[ui].dist_steps),
            static_cast<unsigned>(slot[ui].riv_done));
        delete slot[ui].near;
        delete slot[ui].river;
        delete slot[ui].distant;
    }
    std::printf("frames: %s\n", NE_FRAMES);
    std::printf("result: %s\n", NE_OUT);
    if (verbose) {
        std::printf("*** PASSED near_explore_mk1_hybrid\n");
    }
    delete[] exp;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
