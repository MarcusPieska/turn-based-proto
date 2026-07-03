//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cerrno>

#include <sys/stat.h>
#include <sys/types.h>

#include "walk_coast_mk1.h"
#include "river_explore_mk2.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "map_bit_overlay.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const cstr WH_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const cstr WH_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const cstr WH_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_rivers.ppm";
static const cstr WH_OUT = "/home/w/Projects/simple-map-gen/walk_coast_mk1_hybrid1_result.ppm";
static const cstr WH_FRAMES = "/home/w/Projects/simple-map-gen/walk_coast_mk1_hybrid1_frames";
static const u16 WH_TURNS = PATH_MP_TURN;
static const u16 WH_SIGHT = 3u;
static const u16 WH_RIV_MOVES = 4u;
static const u16 WH_UNIT2_TURN = 21u;
static const u32 WH_UNIT_N = 2u;
static const u8 WH_RIV_DONE = 3u;
static const u8 k_st_coast = 0u;
static const u8 k_st_river = 1u;

//================================================================================================================================
//=> - HybridSlot -
//================================================================================================================================

struct HybridSlot {
    WalkCoastMk1* coast;
    RiverExploreMk2* river;
    u8 st;
    u8 player;
    bool live;
    u16 px;
    u16 py;
    bool block_riv;
    u32 coast_steps;
    u32 riv_steps;
    u32 riv_done;
};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_ocean (u8 t) {
    return t == TERR_OCEAN[0];
}

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_land (u8 t) {
    if (is_water(t)) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

static bool is_coast (u8 t) {
    return t == TERR_COASTAL[0];
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

static bool build_glob_oc (const GameArraySimple& map, u8* glob) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tile_n = map.tile_n();
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < tile_n; ++i) {
        glob[i] = 0u;
    }
    u32 qn = 0u;
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (!is_ocean(map.get_terrain(x, y))) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            glob[i] = 1u;
            q[qn++] = i;
        }
    }
    static const i16 k_dx4[4] = {0, 1, 0, -1};
    static const i16 k_dy4[4] = {-1, 0, 1, 0};
    for (u32 qh = 0u; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0u; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_water(map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (glob[ni] != 0u) {
                continue;
            }
            glob[ni] = 1u;
            q[qn++] = ni;
        }
    }
    delete[] q;
    return qn > 0u;
}

static bool has_glob_coast_nbr (
    const GameArraySimple& map,
    const u8* glob,
    u16 x,
    u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    static const i16 k_dx4[4] = {0, 1, 0, -1};
    static const i16 k_dy4[4] = {-1, 0, 1, 0};
    for (u32 k = 0u; k < 4u; ++k) {
        const i32 xi = static_cast<i32>(x) + k_dx4[k];
        const i32 yi = static_cast<i32>(y) + k_dy4[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(xi);
        const u16 ay = static_cast<u16>(yi);
        if (ax >= w || ay >= h) {
            continue;
        }
        if (!is_coast(map.get_terrain(ax, ay))) {
            continue;
        }
        if (glob[tidx(w, ax, ay)] != 0u) {
            return true;
        }
    }
    return false;
}

static bool is_glob_walk_spawn (
    const GameArraySimple& map,
    const u8* glob,
    u16 x,
    u16 y) {
    return is_land(map.get_terrain(x, y)) && has_glob_coast_nbr(map, glob, x, y);
}

static bool pick_land_at_water (
    const GameArraySimple& map,
    const u8* glob,
    u16 wx,
    u16 wy,
    u16 px,
    u16 py,
    u16& ox,
    u16& oy) {
    if (is_glob_walk_spawn(map, glob, px, py)) {
        ox = px;
        oy = py;
        return true;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    static const i16 k_dx4[4] = {0, 1, 0, -1};
    static const i16 k_dy4[4] = {-1, 0, 1, 0};
    for (u32 k = 0u; k < 4u; ++k) {
        const i32 xi = static_cast<i32>(wx) + k_dx4[k];
        const i32 yi = static_cast<i32>(wy) + k_dy4[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(xi);
        const u16 ay = static_cast<u16>(yi);
        if (ax >= w || ay >= h) {
            continue;
        }
        if (is_glob_walk_spawn(map, glob, ax, ay)) {
            ox = ax;
            oy = ay;
            return true;
        }
    }
    if (is_glob_walk_spawn(map, glob, wx, wy)) {
        ox = wx;
        oy = wy;
        return true;
    }
    return false;
}

static bool scan_card_oc (
    const GameArraySimple& map,
    const u8* glob,
    u16 cx,
    u16 cy,
    i16 dx,
    i16 dy,
    u16& ox,
    u16& oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    u16 px = cx;
    u16 py = cy;
    while (true) {
        const i32 nx = static_cast<i32>(px) + dx;
        const i32 ny = static_cast<i32>(py) + dy;
        if (nx < 0 || ny < 0) {
            return false;
        }
        const u16 x = static_cast<u16>(nx);
        const u16 y = static_cast<u16>(ny);
        if (x >= w || y >= h) {
            return false;
        }
        if (glob[tidx(w, x, y)] != 0u) {
            return pick_land_at_water(map, glob, x, y, px, py, ox, oy);
        }
        px = x;
        py = y;
    }
}

static bool find_glob_coast_spawn (const GameArraySimple& map, const u8* glob, u16& ox, u16& oy) {
    const u16 cx = map.width() / 2u;
    const u16 cy = map.height() / 2u;
    static const i16 k_dx[4] = {0, 1, 0, -1};
    static const i16 k_dy[4] = {-1, 0, 1, 0};
    for (u32 d = 0u; d < 4u; ++d) {
        if (scan_card_oc(map, glob, cx, cy, k_dx[d], k_dy[d], ox, oy)) {
            return true;
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

static bool load_terr_rgb (cstr path, u16 ew, u16 eh, u8** out_rgb) {
    u16 w = 0;
    u16 h = 0;
    u8* rgb = nullptr;
    FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    char magic[3] = {};
    if (std::fscanf(fp, "%2s", magic) != 1 || magic[0] != 'P' || magic[1] != '6') {
        std::fclose(fp);
        return false;
    }
    int c = std::fgetc(fp);
    while (c == '#') {
        while (c != '\n' && c != EOF) {
            c = std::fgetc(fp);
        }
        c = std::fgetc(fp);
    }
    ungetc(c, fp);
    unsigned wi = 0;
    unsigned hi = 0;
    unsigned maxv = 0;
    if (std::fscanf(fp, "%u %u %u", &wi, &hi, &maxv) != 3 || maxv != 255u) {
        std::fclose(fp);
        return false;
    }
    c = std::fgetc(fp);
    if (c == EOF || wi == 0u || hi == 0u || wi > 65535u || hi > 65535u) {
        std::fclose(fp);
        return false;
    }
    w = static_cast<u16>(wi);
    h = static_cast<u16>(hi);
    if (w != ew || h != eh) {
        std::fclose(fp);
        return false;
    }
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    rgb = new u8[nbytes];
    if (rgb == nullptr) {
        std::fclose(fp);
        return false;
    }
    if (std::fread(rgb, 1, nbytes, fp) != nbytes) {
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    *out_rgb = rgb;
    return true;
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

static void frame_path (char* path, size_t n, cstr dir, u16 turn) {
    std::snprintf(path, n, "%s/%05u.ppm", dir, static_cast<unsigned>(turn));
}

static u32 cnt_exp (const u8* exp, u32 n) {
    u32 c = 0u;
    for (u32 i = 0u; i < n; ++i) {
        c += exp[i];
    }
    return c;
}

static void grad_rgb (u8 d, u8 mx, u8& r, u8& g, u8& b) {
    if (d == 0u) {
        r = 255;
        g = 0;
        b = 0;
        return;
    }
    const u32 span = (mx > 0u) ? static_cast<u32>(mx) : 1u;
    const u32 t = (static_cast<u32>(d) * 255u) / span;
    r = static_cast<u8>(255u - t);
    g = static_cast<u8>(220u - (t * 3u) / 4u);
    b = static_cast<u8>(t);
}

static void mark_grad (u8* rgb, u16 w, u16 h, const WalkCoastMk1& ai) {
    if (ai.phase() != 1u) {
        return;
    }
    const u8 mx = ai.grad_max();
    u16 gi = 0u;
    u16 gx = 0u;
    u16 gy = 0u;
    u8 d = 0u;
    while (ai.grad_tile(gi, gx, gy, d)) {
        u8 r = 0u;
        u8 g = 0u;
        u8 b = 0u;
        grad_rgb(d, mx, r, g, b);
        mark_pt(rgb, w, h, gx, gy, r, g, b);
        ++gi;
    }
}

static void mark_riv_tiles (
    u8* rgb,
    u16 w,
    u16 h,
    const GameArraySimple& map,
    const u8* exp) {
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            if (map.get_river(x, y) == 0u) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            if (exp[i] == 0u) {
                continue;
            }
            rgb[i * 3u + 0] = 40;
            rgb[i * 3u + 1] = 80;
            rgb[i * 3u + 2] = 200;
        }
    }
}

static void mark_unit (u8* rgb, u16 w, u16 h, const HybridSlot& u, u8 id) {
    if (u.st == k_st_river && u.river != nullptr) {
        const u8 ph = u.river->phase();
        if (id == 0u) {
            if (ph == 1u) {
                mark_pt(rgb, w, h, u.px, u.py, 0, 255, 0);
            } else if (ph == 2u) {
                mark_pt(rgb, w, h, u.px, u.py, 255, 128, 0);
            } else {
                mark_pt(rgb, w, h, u.px, u.py, 200, 0, 200);
            }
        } else if (ph == 1u) {
            mark_pt(rgb, w, h, u.px, u.py, 128, 255, 128);
        } else if (ph == 2u) {
            mark_pt(rgb, w, h, u.px, u.py, 255, 200, 128);
        } else {
            mark_pt(rgb, w, h, u.px, u.py, 255, 128, 255);
        }
        return;
    }
    if (u.coast != nullptr) {
        if (id == 0u) {
            if (u.coast->phase() == 0u) {
                mark_pt(rgb, w, h, u.px, u.py, 255, 255, 0);
            } else {
                mark_pt(rgb, w, h, u.px, u.py, 0, 255, 255);
            }
        } else if (u.coast->phase() == 0u) {
            mark_pt(rgb, w, h, u.px, u.py, 255, 128, 0);
        } else {
            mark_pt(rgb, w, h, u.px, u.py, 255, 0, 255);
        }
    }
}

static bool save_frame_ppm (
    const u8* terr_rgb,
    const u8* exp,
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u16 sx0,
    u16 sy0,
    u16 sx1,
    u16 sy1,
    bool u1_on,
    const HybridSlot* slot,
    u32 slot_n,
    cstr path) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0u; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (exp[i] != 0u) {
            r = terr_rgb[i * 3u + 0];
            g = terr_rgb[i * 3u + 1];
            b = terr_rgb[i * 3u + 2];
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    mark_riv_tiles(rgb, w, h, map, exp);
    for (u32 ui = 0u; ui < slot_n; ++ui) {
        if (!slot[ui].live) {
            continue;
        }
        if (slot[ui].st == k_st_coast && slot[ui].coast != nullptr) {
            mark_grad(rgb, w, h, *slot[ui].coast);
        }
    }
    mark_cross(rgb, w, h, sx0, sy0, 255, 255, 255);
    if (u1_on) {
        mark_cross(rgb, w, h, sx1, sy1, 0, 255, 0);
    }
    for (u32 ui = 0u; ui < slot_n; ++ui) {
        if (!slot[ui].live) {
            continue;
        }
        mark_unit(rgb, w, h, slot[ui], static_cast<u8>(ui));
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

static bool coast_on_riv (const GameArraySimple& map, const WalkCoastMk1& coast) {
    return coast.phase() == 0u && map.get_river(coast.x(), coast.y()) != 0u;
}

static void start_river (
    HybridSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sight) {
    sync_exp_ov(exp, ov, map.width(), map.height());
    u.river = new RiverExploreMk2(map, ov, u.px, u.py, sight, u.player);
    sync_ov_exp(exp, ov, map.width(), map.height());
    delete u.coast;
    u.coast = nullptr;
    u.st = k_st_river;
}

static void resume_coast (
    HybridSlot& u,
    const GameArraySimple& map,
    u8* exp,
    u16 sight) {
    u.coast = new WalkCoastMk1(map, exp, u.px, u.py, sight, u.player);
    u.st = k_st_coast;
    u.block_riv = true;
}

static void try_riv_handoff (
    HybridSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sight,
    bool verbose) {
    if (u.coast == nullptr || u.coast->done() || u.block_riv) {
        return;
    }
    if (!coast_on_riv(map, *u.coast)) {
        return;
    }
    u.px = u.coast->x();
    u.py = u.coast->y();
    if (verbose) {
        std::printf("  u%u coast->river at (%u,%u)\n",
            static_cast<unsigned>(u.player),
            static_cast<unsigned>(u.px),
            static_cast<unsigned>(u.py));
    }
    start_river(u, map, exp, ov, sight);
}

static void tick_hybrid (
    HybridSlot& u,
    const GameArraySimple& map,
    u8* exp,
    MapBitOverlay& ov,
    u16 sight,
    bool verbose) {
    if (u.st == k_st_coast) {
        if (u.coast == nullptr) {
            return;
        }
        try_riv_handoff(u, map, exp, ov, sight, verbose);
        if (u.st == k_st_river) {
            return;
        }
        if (u.coast->done()) {
            u.px = u.coast->x();
            u.py = u.coast->y();
            return;
        }
        const u16 ox = u.coast->x();
        const u16 oy = u.coast->y();
        u.coast->move(1u);
        u.px = u.coast->x();
        u.py = u.coast->y();
        if (ox != u.px || oy != u.py) {
            ++u.coast_steps;
        }
        if (map.get_river(u.px, u.py) == 0u) {
            u.block_riv = false;
        }
        try_riv_handoff(u, map, exp, ov, sight, verbose);
        return;
    }
    if (u.st == k_st_river) {
        if (u.river == nullptr) {
            return;
        }
        for (u16 m = 0u; m < WH_RIV_MOVES; ++m) {
            if (u.river->phase() == WH_RIV_DONE) {
                break;
            }
            const u16 ox = u.river->x();
            const u16 oy = u.river->y();
            u.river->move(1u);
            u.px = u.river->x();
            u.py = u.river->y();
            if (ox != u.px || oy != u.py) {
                ++u.riv_steps;
            }
        }
        sync_ov_exp(exp, ov, map.width(), map.height());
        if (u.river->phase() != WH_RIV_DONE) {
            return;
        }
        ++u.riv_done;
        if (verbose) {
            std::printf("  u%u river->coast at (%u,%u) terr %u\n",
                static_cast<unsigned>(u.player),
                static_cast<unsigned>(u.px),
                static_cast<unsigned>(u.py),
                static_cast<unsigned>(map.get_terrain(u.px, u.py)));
        }
        delete u.river;
        u.river = nullptr;
        resume_coast(u, map, exp, sight);
    }
}

static bool hybrid_done (const HybridSlot& u) {
    return u.st == k_st_coast && u.coast != nullptr && u.coast->done();
}

static bool all_done (const HybridSlot* slot, u32 n) {
    for (u32 i = 0u; i < n; ++i) {
        if (!slot[i].live) {
            continue;
        }
        if (!hybrid_done(slot[i])) {
            return false;
        }
    }
    return true;
}

static void spawn_slot (
    HybridSlot& u,
    const GameArraySimple& map,
    u8* exp,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player) {
    u.coast = new WalkCoastMk1(map, exp, sx, sy, sight, player);
    u.river = nullptr;
    u.st = k_st_coast;
    u.player = player;
    u.live = true;
    u.px = sx;
    u.py = sy;
    u.block_riv = false;
    u.coast_steps = 0u;
    u.riv_steps = 0u;
    u.riv_done = 0u;
}

static void free_slot (HybridSlot& u) {
    delete u.river;
    delete u.coast;
    u.river = nullptr;
    u.coast = nullptr;
    u.live = false;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool verbose = (argc >= 2 && argv[1] != nullptr && std::atoi(argv[1]) >= 2);
    GameArraySimple map;
    if (!Factory_GameArraySimple::load(&map, WH_IN_TERR, WH_IN_CLIM, WH_IN_RIV)) {
        std::printf("*** FAILED load map\n");
        return 1;
    }
    if (!ensure_dir(WH_FRAMES)) {
        std::printf("*** FAILED mkdir %s\n", WH_FRAMES);
        return 1;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    const u32 tile_n = map.tile_n();
    u8* glob = new u8[tile_n];
    if (glob == nullptr) {
        std::printf("*** FAILED alloc glob\n");
        return 1;
    }
    if (!build_glob_oc(map, glob)) {
        std::printf("*** FAILED global ocean flood\n");
        delete[] glob;
        return 1;
    }
    if (!find_glob_coast_spawn(map, glob, sx, sy)) {
        std::printf("*** FAILED find global coast spawn\n");
        delete[] glob;
        return 1;
    }
    delete[] glob;
    u8* terr_rgb = nullptr;
    if (!load_terr_rgb(WH_IN_TERR, map.width(), map.height(), &terr_rgb)) {
        std::printf("*** FAILED load terrain rgb\n");
        return 1;
    }
    u8* exp = new u8[tile_n];
    if (exp == nullptr) {
        std::printf("*** FAILED alloc exp\n");
        delete[] terr_rgb;
        return 1;
    }
    for (u32 i = 0u; i < tile_n; ++i) {
        exp[i] = 0u;
    }
    const u16 map_w = map.width();
    const u16 map_h = map.height();
    MapBitOverlay ov(map_w, map_h);
    HybridSlot slot[WH_UNIT_N] = {};
    spawn_slot(slot[0], map, exp, sx, sy, WH_SIGHT, 0u);
    char path[160];
    frame_path(path, sizeof(path), WH_FRAMES, 0u);
    if (!save_frame_ppm(terr_rgb, exp, map, map_w, map_h, sx, sy, sx, sy, false, slot, WH_UNIT_N, path)) {
        std::printf("*** FAILED save frame %u\n", 0u);
        free_slot(slot[0]);
        delete[] terr_rgb;
        delete[] exp;
        return 1;
    }
    if (verbose) {
        std::printf("walk_coast_mk1_hybrid1: %ux%u sight %u spawn (%u,%u)\n",
            static_cast<unsigned>(map_w),
            static_cast<unsigned>(map_h),
            static_cast<unsigned>(WH_SIGHT),
            static_cast<unsigned>(sx),
            static_cast<unsigned>(sy));
    }
    u16 turn = 0u;
    while (turn < WH_TURNS && !all_done(slot, WH_UNIT_N)) {
        ++turn;
        if (turn == WH_UNIT2_TURN && !slot[1].live) {
            spawn_slot(slot[1], map, exp, sx, sy, WH_SIGHT, 1u);
            if (verbose) {
                std::printf("  turn %u spawn unit 1 at (%u,%u)\n",
                    static_cast<unsigned>(turn),
                    static_cast<unsigned>(sx),
                    static_cast<unsigned>(sy));
            }
        }
        for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
            if (!slot[ui].live || hybrid_done(slot[ui])) {
                continue;
            }
            tick_hybrid(slot[ui], map, exp, ov, WH_SIGHT, verbose);
        }
        frame_path(path, sizeof(path), WH_FRAMES, turn);
        if (!save_frame_ppm(terr_rgb, exp, map, map_w, map_h, sx, sy, sx, sy, slot[1].live, slot, WH_UNIT_N, path)) {
            std::printf("*** FAILED save frame %u\n", static_cast<unsigned>(turn));
            for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
                free_slot(slot[ui]);
            }
            delete[] terr_rgb;
            delete[] exp;
            return 1;
        }
        if (verbose && (turn == 1u || (turn % 100u) == 0u)) {
            std::printf("  turn %u explored %u",
                static_cast<unsigned>(turn),
                static_cast<unsigned>(cnt_exp(exp, tile_n)));
            for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
                if (!slot[ui].live) {
                    continue;
                }
                std::printf(" u%u st %u (%u,%u) coast %u riv %u rivers %u",
                    static_cast<unsigned>(ui),
                    static_cast<unsigned>(slot[ui].st),
                    static_cast<unsigned>(slot[ui].px),
                    static_cast<unsigned>(slot[ui].py),
                    static_cast<unsigned>(slot[ui].coast_steps),
                    static_cast<unsigned>(slot[ui].riv_steps),
                    static_cast<unsigned>(slot[ui].riv_done));
            }
            std::printf("\n");
        }
    }
    if (!save_frame_ppm(terr_rgb, exp, map, map_w, map_h, sx, sy, sx, sy, slot[1].live, slot, WH_UNIT_N, WH_OUT)) {
        std::printf("*** FAILED save %s\n", WH_OUT);
        for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
            free_slot(slot[ui]);
        }
        delete[] terr_rgb;
        delete[] exp;
        return 1;
    }
    std::printf("turns %u frames %u explored %u\n",
        static_cast<unsigned>(turn),
        static_cast<unsigned>(turn),
        static_cast<unsigned>(cnt_exp(exp, tile_n)));
    for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
        if (!slot[ui].live) {
            continue;
        }
        std::printf("u%u end st %u pos (%u,%u) coast %u riv %u rivers %u",
            static_cast<unsigned>(ui),
            static_cast<unsigned>(slot[ui].st),
            static_cast<unsigned>(slot[ui].px),
            static_cast<unsigned>(slot[ui].py),
            static_cast<unsigned>(slot[ui].coast_steps),
            static_cast<unsigned>(slot[ui].riv_steps),
            static_cast<unsigned>(slot[ui].riv_done));
        if (slot[ui].coast != nullptr) {
            std::printf(" coast_ph %u done %u loc_exh %u",
                static_cast<unsigned>(slot[ui].coast->phase()),
                static_cast<unsigned>(slot[ui].coast->done() ? 1u : 0u),
                static_cast<unsigned>(slot[ui].coast->loc_exh() ? 1u : 0u));
        }
        std::printf("\n");
    }
    std::printf("frames: %s\n", WH_FRAMES);
    std::printf("result: %s\n", WH_OUT);
    if (verbose) {
        std::printf("*** PASSED walk_coast_mk1_hybrid1\n");
    }
    for (u32 ui = 0u; ui < WH_UNIT_N; ++ui) {
        free_slot(slot[ui]);
    }
    delete[] terr_rgb;
    delete[] exp;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
