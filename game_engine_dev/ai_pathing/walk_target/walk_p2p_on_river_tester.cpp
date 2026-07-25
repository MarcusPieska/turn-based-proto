//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "generate_distance_p2p.h"
#include "runtime_static_loader.h"
#include "unit_movement_mng.h"
#include "walk_p2p.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const cstr G_TERR = "/home/w/Projects/simple-map-gen/p1-seed-43/terrain.ppm";
static const cstr G_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-43/climate.ppm";
static const cstr G_RIV = "/home/w/Projects/simple-map-gen/p1-seed-43/rivers.ppm";
static const cstr G_RES = "/home/w/Projects/simple-map-gen/p1-seed-43/overlay.ppm";
static const cstr G_OUT_DIR = "/home/w/Projects/simple-map-gen/distance-p2p-on-river";
static const cstr G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const cstr G_DATA = "../../";

static const u16 k_dep_none = 0xFFFFu;
static const u16 k_turn_sent = GenerateDistanceP2P::k_turn_sent;
static const u32 k_step_max = 200000u;
static const u32 k_pair_n = 100u;
static const u32 k_min_area = 64u;
static const u16 k_min_depth = 16u;

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool is_land (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_wtr(t);
}

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load(G_LIB, G_DATA)) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool init_state (GameState* state) {
    if (state == nullptr || !load_statics()) {
        return false;
    }
    state->clear();
    state->m_statics = g_rt_statics;
    if (!UnitMovementMng::setup_mvt_costs(*g_rt_statics)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, G_TERR, G_CLIM, G_RIV)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, G_RES)) {
        return false;
    }
    return state->m_map.width() > 0 && state->m_map.height() > 0;
}

static void flatten_inland (GameState& s) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            GameTileSimple* t = s.m_map.tile(x, y);
            if (t == nullptr) {
                continue;
            }
            if (t->m_terr == TERR_INLAND_SEA[0] || t->m_terr == TERR_INLAND_LAKE[0]) {
                t->m_terr = TERR_PLAINS[0];
            }
        }
    }
    (void)w;
}

static bool riv_walk (const GameState& s, u16 x, u16 y) {
    return s.m_map.get_river(x, y) != 0u && is_land(s.m_map.get_terrain(x, y));
}

static bool ensure_out_dir () {
    char cmd[512];
    std::snprintf(cmd, sizeof(cmd), "mkdir -p %s", G_OUT_DIR);
    return std::system(cmd) == 0;
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (u32)w, (u32)h);
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, n, fp) == n;
    std::fclose(fp);
    return ok;
}

//================================================================================================================================
//=> - RivSysIdx -
//================================================================================================================================

class RivSysIdx {
public:
    RivSysIdx () :
        m_id(nullptr),
        m_area(nullptr),
        m_depth(nullptr),
        m_ok_sys(nullptr),
        m_w(0),
        m_h(0),
        m_n(0),
        m_ok_n(0) {
    }

    ~RivSysIdx () {
        delete[] m_id;
        delete[] m_area;
        delete[] m_depth;
        delete[] m_ok_sys;
    }

    bool build (const GameState& s) {
        delete[] m_id;
        delete[] m_area;
        delete[] m_depth;
        delete[] m_ok_sys;
        m_id = nullptr;
        m_area = nullptr;
        m_depth = nullptr;
        m_ok_sys = nullptr;
        m_n = 0;
        m_ok_n = 0;
        m_w = s.m_map.width();
        m_h = s.m_map.height();
        const u32 tn = s.m_map.tile_n();
        m_id = new u16[tn];
        u32* q = new u32[tn];
        u16* dep = new u16[tn];
        for (u32 i = 0; i < tn; ++i) {
            m_id[i] = 0;
            dep[i] = k_dep_none;
        }
        u16 sid = 0;
        for (u16 y = 0; y < m_h; ++y) {
            for (u16 x = 0; x < m_w; ++x) {
                const u32 si = tidx(m_w, x, y);
                if (m_id[si] != 0u || !riv_walk(s, x, y)) {
                    continue;
                }
                if (sid == 0xFFFFu) {
                    delete[] q;
                    delete[] dep;
                    return false;
                }
                sid++;
                u32 qn = 0;
                u16 max_d = 0;
                m_id[si] = sid;
                dep[si] = 0;
                q[qn++] = si;
                for (u32 qi = 0; qi < qn; ++qi) {
                    const u32 ci = q[qi];
                    const u16 cx = static_cast<u16>(ci % m_w);
                    const u16 cy = static_cast<u16>(ci / m_w);
                    const u16 cd = dep[ci];
                    if (cd > max_d) {
                        max_d = cd;
                    }
                    for (u32 k = 0; k < MAP_NBR4_N; ++k) {
                        const i32 nx = static_cast<i32>(cx) + MAP_NBR4_DX[k];
                        const i32 ny = static_cast<i32>(cy) + MAP_NBR4_DY[k];
                        if (nx < 0 || ny < 0) {
                            continue;
                        }
                        const u16 tx = static_cast<u16>(nx);
                        const u16 ty = static_cast<u16>(ny);
                        if (tx >= m_w || ty >= m_h) {
                            continue;
                        }
                        const u32 ni = tidx(m_w, tx, ty);
                        if (m_id[ni] != 0u || !riv_walk(s, tx, ty)) {
                            continue;
                        }
                        m_id[ni] = sid;
                        const u32 nd = static_cast<u32>(cd) + 1u;
                        dep[ni] = nd > 65534u ? 65534u : static_cast<u16>(nd);
                        q[qn++] = ni;
                    }
                }
                if (m_area == nullptr) {
                    m_area = new u32[1];
                    m_depth = new u16[1];
                } else {
                    u32* na = new u32[sid];
                    u16* ndp = new u16[sid];
                    std::memcpy(na, m_area, sizeof(u32) * (sid - 1u));
                    std::memcpy(ndp, m_depth, sizeof(u16) * (sid - 1u));
                    delete[] m_area;
                    delete[] m_depth;
                    m_area = na;
                    m_depth = ndp;
                }
                m_area[sid - 1u] = qn;
                m_depth[sid - 1u] = max_d;
            }
        }
        delete[] q;
        delete[] dep;
        m_n = sid;
        m_ok_n = 0;
        if (m_n == 0) {
            return false;
        }
        m_ok_sys = new u16[m_n];
        for (u16 i = 0; i < m_n; ++i) {
            if (m_area[i] >= k_min_area && m_depth[i] >= k_min_depth) {
                m_ok_sys[m_ok_n++] = static_cast<u16>(i + 1u);
            }
        }
        return m_ok_n > 0u;
    }

    u16 sys_at (u16 x, u16 y) const {
        if (m_id == nullptr || x >= m_w || y >= m_h) {
            return 0;
        }
        return m_id[tidx(m_w, x, y)];
    }

    u16 ok_n () const {
        return m_ok_n;
    }

    u16 ok_sys (u16 i) const {
        return m_ok_sys[i];
    }

    u32 area (u16 sid) const {
        if (sid == 0 || sid > m_n) {
            return 0;
        }
        return m_area[sid - 1u];
    }

    bool pick_pair (
        const GameState& s,
        u16 sid,
        u32 salt,
        u16* sx,
        u16* sy,
        u16* dx,
        u16* dy) const {
        if (sid == 0 || sid > m_n || m_id == nullptr) {
            return false;
        }
        const u32 tn = static_cast<u32>(m_w) * static_cast<u32>(m_h);
        u32* tiles = new u32[tn];
        u32 nn = 0;
        for (u32 i = 0; i < tn; ++i) {
            if (m_id[i] == sid) {
                tiles[nn++] = i;
            }
        }
        if (nn < 2u) {
            delete[] tiles;
            return false;
        }
        const u32 seed_i = tiles[salt % nn];
        u16* dep = new u16[tn];
        u32* q = new u32[tn];
        for (u32 i = 0; i < tn; ++i) {
            dep[i] = k_dep_none;
        }
        u32 qn = 0;
        dep[seed_i] = 0;
        q[qn++] = seed_i;
        for (u32 qi = 0; qi < qn; ++qi) {
            const u32 ci = q[qi];
            const u16 cx = static_cast<u16>(ci % m_w);
            const u16 cy = static_cast<u16>(ci / m_w);
            const u16 cd = dep[ci];
            for (u32 k = 0; k < MAP_NBR4_N; ++k) {
                const i32 nx = static_cast<i32>(cx) + MAP_NBR4_DX[k];
                const i32 ny = static_cast<i32>(cy) + MAP_NBR4_DY[k];
                if (nx < 0 || ny < 0) {
                    continue;
                }
                const u16 tx = static_cast<u16>(nx);
                const u16 ty = static_cast<u16>(ny);
                if (tx >= m_w || ty >= m_h) {
                    continue;
                }
                const u32 ni = tidx(m_w, tx, ty);
                if (dep[ni] != k_dep_none || m_id[ni] != sid) {
                    continue;
                }
                const u32 nd = static_cast<u32>(cd) + 1u;
                dep[ni] = nd > 65534u ? 65534u : static_cast<u16>(nd);
                q[qn++] = ni;
            }
        }
        u16 best = 0;
        u32 far_i = seed_i;
        bool got = false;
        for (u32 i = 0; i < qn; ++i) {
            const u32 ci = q[i];
            const u16 d = dep[ci];
            if (d == 0u) {
                continue;
            }
            if (!got || d > best) {
                best = d;
                far_i = ci;
                got = true;
            }
        }
        *dx = static_cast<u16>(seed_i % m_w);
        *dy = static_cast<u16>(seed_i / m_w);
        *sx = static_cast<u16>(far_i % m_w);
        *sy = static_cast<u16>(far_i / m_w);
        delete[] dep;
        delete[] q;
        delete[] tiles;
        (void)s;
        return got;
    }

private:
    RivSysIdx (const RivSysIdx& other) = delete;
    RivSysIdx& operator= (const RivSysIdx& other) = delete;

    u16* m_id;
    u32* m_area;
    u16* m_depth;
    u16* m_ok_sys;
    u16 m_w;
    u16 m_h;
    u16 m_n;
    u16 m_ok_n;
};

//================================================================================================================================
//=> - Walk / image -
//================================================================================================================================

static const u8 k_path_off = 0u;
static const u8 k_path_oth = 1u;
static const u8 k_path_riv = 2u;

static bool sim_walk (
    WalkP2P& walk,
    const GameState& s,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u8* path_m,
    u32* steps,
    u32* turns,
    u32* riv_n,
    u32* oth_n) {
    const u16 w = s.m_map.width();
    const u32 tn = s.m_map.tile_n();
    std::memset(path_m, 0, tn);
    const u16 mp_turn = g_rt_statics->config().get_mov_pt_per_turn();
    i32 mp = static_cast<i32>(mp_turn);
    u16 x = sx;
    u16 y = sy;
    *steps = 0;
    *turns = 1;
    *riv_n = 0;
    *oth_n = 0;
    path_m[tidx(w, x, y)] = k_path_oth;
    while (!(x == dx && y == dy)) {
        if (*steps >= k_step_max) {
            return false;
        }
        while (mp <= 0) {
            mp += static_cast<i32>(mp_turn);
            (*turns)++;
            if (*turns > 100000u) {
                return false;
            }
        }
        const WalkP2P::StepRes r = walk.peek(s, x, y);
        if (!r.have) {
            return false;
        }
        const bool rr = s.m_map.get_river(x, y) != 0u && s.m_map.get_river(r.nx, r.ny) != 0u;
        mp -= static_cast<i32>(r.cost);
        x = r.nx;
        y = r.ny;
        if (rr) {
            path_m[tidx(w, x, y)] = k_path_riv;
            (*riv_n)++;
        } else {
            path_m[tidx(w, x, y)] = k_path_oth;
            (*oth_n)++;
        }
        (*steps)++;
    }
    return true;
}

static void terr_rgb (const GameState& s, u16 x, u16 y, u8* r, u8* g, u8* b) {
    const u8 t = s.m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1];
        *g = TERR_OCEAN[2];
        *b = TERR_OCEAN[3];
        return;
    }
    if (t == TERR_SEA[0]) {
        *r = TERR_SEA[1];
        *g = TERR_SEA[2];
        *b = TERR_SEA[3];
        return;
    }
    if (t == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1];
        *g = TERR_COASTAL[2];
        *b = TERR_COASTAL[3];
        return;
    }
    if (t == TERR_HILLS[0]) {
        *r = TERR_HILLS[1];
        *g = TERR_HILLS[2];
        *b = TERR_HILLS[3];
        return;
    }
    if (t == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1];
        *g = TERR_MOUNTAINS[2];
        *b = TERR_MOUNTAINS[3];
        return;
    }
    *r = TERR_PLAINS[1];
    *g = TERR_PLAINS[2];
    *b = TERR_PLAINS[3];
}

static void build_img (
    const GameState& s,
    const u8* path_m,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u8* rgb) {
    auto put = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        u8* px = rgb + tidx(w, x, y) * 3u;
        px[0] = r;
        px[1] = g;
        px[2] = b;
    };
    auto blob = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        for (i32 oy = -1; oy <= 1; ++oy) {
            for (i32 ox = -1; ox <= 1; ++ox) {
                const i32 nx = static_cast<i32>(x) + ox;
                const i32 ny = static_cast<i32>(y) + oy;
                if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                    continue;
                }
                put(static_cast<u16>(nx), static_cast<u16>(ny), r, g, b);
            }
        }
    };
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(s, x, y, &r, &g, &b);
            if (s.m_map.get_river(x, y) != 0u && path_m[i] == k_path_off) {
                r = static_cast<u8>((static_cast<u16>(r) * 2u + 40u) / 3u);
                g = static_cast<u8>((static_cast<u16>(g) * 2u + 120u) / 3u);
                b = static_cast<u8>((static_cast<u16>(b) * 2u + 255u) / 3u);
            }
            if (path_m[i] != k_path_off) {
                if (s.m_map.get_river(x, y) != 0u) {
                    r = 0;
                    g = 0;
                    b = 0;
                } else {
                    r = 255;
                    g = 255;
                    b = 255;
                }
            }
            put(x, y, r, g, b);
        }
    }
    blob(sx, sy, 255, 0, 0);
    blob(dx, dy, 0, 255, 0);
}

static void build_grad_img (
    const GameState& s,
    const u16* dist,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u16 dmax,
    u8* rgb) {
    auto put = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        u8* px = rgb + tidx(w, x, y) * 3u;
        px[0] = r;
        px[1] = g;
        px[2] = b;
    };
    auto blob = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        for (i32 oy = -1; oy <= 1; ++oy) {
            for (i32 ox = -1; ox <= 1; ++ox) {
                const i32 nx = static_cast<i32>(x) + ox;
                const i32 ny = static_cast<i32>(y) + oy;
                if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                    continue;
                }
                put(static_cast<u16>(nx), static_cast<u16>(ny), r, g, b);
            }
        }
    };
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            const u8 t = s.m_map.get_terrain(x, y);
            if (is_wtr(t)) {
                put(x, y, 30, 110, 220);
                continue;
            }
            if (t == TERR_MOUNTAINS[0]) {
                put(x, y, 76, 48, 30);
                continue;
            }
            const u16 d = dist[i];
            if (d == k_turn_sent) {
                put(x, y, 40, 40, 40);
                continue;
            }
            const u32 span = dmax == 0u ? 1u : static_cast<u32>(dmax);
            const u8 v = static_cast<u8>(55u + (200u * (span - d)) / span);
            put(x, y, v, static_cast<u8>(v / 2u), 40);
        }
    }
    blob(sx, sy, 255, 0, 0);
    blob(dx, dy, 0, 255, 0);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    GameState state;
    if (!init_state(&state)) {
        std::printf("*** FAILED init_state\n");
        return 1;
    }
    flatten_inland(state);
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    WhiteboardMng::init(w, h);
    RivSysIdx riv;
    if (!riv.build(state)) {
        std::printf("*** FAILED riv systems (none large enough)\n");
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("riv ok systems=%u\n", (u32)riv.ok_n());
    if (!ensure_out_dir()) {
        std::printf("*** FAILED mkdir %s\n", G_OUT_DIR);
        WhiteboardMng::terminate();
        return 1;
    }
    u8* path_m = new u8[state.m_map.tile_n()];
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    u32 pass_n = 0;
    u32 fail_n = 0;
    for (u32 pi = 0; pi < k_pair_n; ++pi) {
        const u16 sid = riv.ok_sys(static_cast<u16>(pi % riv.ok_n()));
        u16 sx = 0;
        u16 sy = 0;
        u16 dx = 0;
        u16 dy = 0;
        if (!riv.pick_pair(state, sid, pi * 17u + 3u, &sx, &sy, &dx, &dy)) {
            std::printf("[%03u] FAILED pick sid=%u\n", (u32)pi, (u32)sid);
            fail_n++;
            continue;
        }
        WalkP2P walk;
        if (!walk.ok()) {
            std::printf("[%03u] FAILED WalkP2P checkout\n", (u32)pi);
            fail_n++;
            continue;
        }
        u16 dmax = 0;
        const bool gen_ok = GenerateDistanceP2P::generate(
            state, *g_rt_statics, sx, sy, dx, dy, walk, nullptr, &dmax);
        if (!gen_ok) {
            std::printf("[%03u] FAILED generate src=(%u,%u) dst=(%u,%u) sid=%u\n",
                (u32)pi, (u32)sx, (u32)sy, (u32)dx, (u32)dy, (u32)sid);
            fail_n++;
            continue;
        }
        u32 steps = 0;
        u32 turns = 0;
        u32 riv_n = 0;
        u32 oth_n = 0;
        if (!sim_walk(walk, state, sx, sy, dx, dy, path_m, &steps, &turns, &riv_n, &oth_n)) {
            std::printf("[%03u] FAILED walk src=(%u,%u) dst=(%u,%u)\n", (u32)pi, (u32)sx, (u32)sy, (u32)dx, (u32)dy);
            fail_n++;
            continue;
        }
        build_img(state, path_m, w, h, sx, sy, dx, dy, rgb);
        char path[512];
        std::snprintf(path, sizeof(path), "%s/distance-p2p-on-river-%03u.ppm", G_OUT_DIR, (u32)pi);
        if (!save_ppm(path, rgb, w, h)) {
            std::printf("[%03u] FAILED save %s\n", (u32)pi, path);
            fail_n++;
            continue;
        }
        build_grad_img(state, walk.turn(), w, h, sx, sy, dx, dy, dmax, rgb);
        std::snprintf(path, sizeof(path), "%s/g-distance-p2p-on-river-%03u.ppm", G_OUT_DIR, (u32)pi);
        if (!save_ppm(path, rgb, w, h)) {
            std::printf("[%03u] FAILED save %s\n", (u32)pi, path);
            fail_n++;
            continue;
        }
        std::printf("[%03u] ok sid=%u area=%u steps=%u turns=%u "
            "\033[94mriv2riv=%u\033[0m \033[31mother=%u\033[0m src=(%u,%u) dst=(%u,%u)\n",
            (u32)pi, (u32)sid, (u32)riv.area(sid),
            (u32)steps, (u32)turns, (u32)riv_n, (u32)oth_n,
            (u32)sx, (u32)sy, (u32)dx, (u32)dy);
        pass_n++;
    }
    delete[] rgb;
    delete[] path_m;
    WhiteboardMng::terminate();
    std::printf("done pass=%u fail=%u out=%s\n", (u32)pass_n, (u32)fail_n, G_OUT_DIR);
    if (fail_n > 0u || pass_n != k_pair_n) {
        std::printf("*** FAILED walk_p2p_on_river\n");
        return 1;
    }
    std::printf("*** PASSED walk_p2p_on_river\n");
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
