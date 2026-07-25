//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

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
static const cstr G_OUT = "/home/w/Projects/simple-map-gen/distance-p2p-walk.ppm";
static const cstr G_TRACE = "/home/w/Projects/simple-map-gen/distance-p2p-walk.trace";
static const cstr G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const cstr G_DATA = "../../";

static const u16 k_turn_sent = GenerateDistanceP2P::k_turn_sent;
static const u16 k_dep_none = 0xFFFFu;
static const u32 k_step_max = 200000u;
static const u32 k_cost_hist_n = 64u;
static const u32 k_enter_max = k_step_max + 1u;

struct CostHist {
    u16 cost;
    u32 n;
};

struct EnterRec {
    u32 turn;
    u16 x;
    u16 y;
};

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

static bool has_land_nbr (const GameState& s, u16 w, u16 h, u16 x, u16 y) {
    for (u32 k = 0; k < MAP_NBR4_N; ++k) {
        const i32 nx = static_cast<i32>(x) + MAP_NBR4_DX[k];
        const i32 ny = static_cast<i32>(y) + MAP_NBR4_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        if (is_land(s.m_map.get_terrain(tx, ty))) {
            return true;
        }
    }
    return false;
}

static bool pick_src_dst (const GameState& s, u16* sx, u16* sy, u16* dx, u16* dy) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 tn = s.m_map.tile_n();
    u16* dep = new u16[tn];
    u32* q = new u32[tn];
    for (u32 i = 0; i < tn; ++i) {
        dep[i] = k_dep_none;
    }
    u32 qn = 0;
    u16 seed_x = 0;
    u16 seed_y = 0;
    bool have = false;
    for (u16 y = 0; y < h && !have; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (is_land(s.m_map.get_terrain(x, y)) && has_land_nbr(s, w, h, x, y)) {
                seed_x = x;
                seed_y = y;
                have = true;
                break;
            }
        }
    }
    if (!have) {
        delete[] dep;
        delete[] q;
        return false;
    }
    const u32 si = tidx(w, seed_x, seed_y);
    dep[si] = 0;
    q[qn++] = si;
    for (u32 qi = 0; qi < qn; ++qi) {
        const u32 ci = q[qi];
        const u16 cx = static_cast<u16>(ci % w);
        const u16 cy = static_cast<u16>(ci / w);
        const u16 cd = dep[ci];
        for (u32 k = 0; k < MAP_NBR4_N; ++k) {
            const i32 nx = static_cast<i32>(cx) + MAP_NBR4_DX[k];
            const i32 ny = static_cast<i32>(cy) + MAP_NBR4_DY[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 tx = static_cast<u16>(nx);
            const u16 ty = static_cast<u16>(ny);
            if (tx >= w || ty >= h) {
                continue;
            }
            const u32 ni = tidx(w, tx, ty);
            if (dep[ni] != k_dep_none || !is_land(s.m_map.get_terrain(tx, ty))) {
                continue;
            }
            const u32 nd = static_cast<u32>(cd) + 1u;
            dep[ni] = nd > 65534u ? 65534u : static_cast<u16>(nd);
            q[qn++] = ni;
        }
    }
    u16 best = 0;
    bool got = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 d = dep[tidx(w, x, y)];
            if (d == k_dep_none || d == 0u) {
                continue;
            }
            if (!got || d > best) {
                best = d;
                *sx = x;
                *sy = y;
                got = true;
            }
        }
    }
    *dx = seed_x;
    *dy = seed_y;
    delete[] dep;
    delete[] q;
    return got;
}

static void cost_add (CostHist* hist, u32* hist_n, u16 cost) {
    for (u32 i = 0; i < *hist_n; ++i) {
        if (hist[i].cost == cost) {
            hist[i].n++;
            return;
        }
    }
    if (*hist_n >= k_cost_hist_n) {
        return;
    }
    hist[*hist_n].cost = cost;
    hist[*hist_n].n = 1u;
    (*hist_n)++;
}

static void cost_sort (CostHist* hist, u32 hist_n) {
    for (u32 i = 1; i < hist_n; ++i) {
        CostHist key = hist[i];
        u32 j = i;
        while (j > 0u && hist[j - 1u].cost > key.cost) {
            hist[j] = hist[j - 1u];
            j--;
        }
        hist[j] = key;
    }
}

static void cost_print (const CostHist* hist, u32 hist_n) {
    std::printf("walk tile costs:\n");
    for (u32 i = 0; i < hist_n; ++i) {
        std::printf("  cost=%u count=%u\n",
            (unsigned)hist[i].cost, (unsigned)hist[i].n);
    }
}

static void enter_add (EnterRec* ent, u32* ent_n, u32 turn, u16 x, u16 y) {
    if (*ent_n >= k_enter_max) {
        return;
    }
    ent[*ent_n].turn = turn;
    ent[*ent_n].x = x;
    ent[*ent_n].y = y;
    (*ent_n)++;
}

static bool save_trace (cstr path, const EnterRec* ent, u32 ent_n) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    for (u32 i = 0; i < ent_n; ++i) {
        std::fprintf(fp, "%u:%u:%u\n",
            (unsigned)ent[i].turn, (unsigned)ent[i].x, (unsigned)ent[i].y);
    }
    std::fclose(fp);
    return true;
}

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
    CostHist* hist,
    u32* hist_n,
    EnterRec* ent,
    u32* ent_n) {
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    const u32 tn = s.m_map.tile_n();
    std::memset(path_m, 0, tn);
    const u16 mp_turn = g_rt_statics->config().get_mov_pt_per_turn();
    i32 mp = static_cast<i32>(mp_turn);
    u16 x = sx;
    u16 y = sy;
    *steps = 0;
    *turns = 1;
    *hist_n = 0;
    *ent_n = 0;
    path_m[tidx(w, x, y)] = 1;
    enter_add(ent, ent_n, *turns, x, y);
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
        cost_add(hist, hist_n, r.cost);
        mp -= static_cast<i32>(r.cost);
        x = r.nx;
        y = r.ny;
        path_m[tidx(w, x, y)] = 1;
        enter_add(ent, ent_n, *turns, x, y);
        (*steps)++;
    }
    (void)h;
    return true;
}

static void build_img (
    const GameState& s,
    const u16* dist,
    const u8* path_m,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    u16 dmax,
    u8* rgb) {
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            u8* px = rgb + i * 3u;
            const u8 t = s.m_map.get_terrain(x, y);
            if (is_wtr(t)) {
                px[0] = 30;
                px[1] = 110;
                px[2] = 220;
                continue;
            }
            if (t == TERR_MOUNTAINS[0]) {
                px[0] = 76;
                px[1] = 48;
                px[2] = 30;
                continue;
            }
            if (path_m[i] != 0u) {
                if (s.m_map.get_river(x, y) != 0u) {
                    px[0] = 40;
                    px[1] = 120;
                    px[2] = 255;
                } else if (t == TERR_HILLS[0]) {
                    px[0] = 0;
                    px[1] = 80;
                    px[2] = 0;
                } else {
                    px[0] = 255;
                    px[1] = 255;
                    px[2] = 255;
                }
                continue;
            }
            const u16 d = dist[i];
            if (d == k_turn_sent) {
                px[0] = 40;
                px[1] = 40;
                px[2] = 40;
                continue;
            }
            const u32 span = dmax == 0u ? 1u : static_cast<u32>(dmax);
            const u8 v = static_cast<u8>(55u + (200u * (span - d)) / span);
            px[0] = v;
            px[1] = static_cast<u8>(v / 2u);
            px[2] = 40;
        }
    }
    auto paint = [&](u16 x, u16 y, u8 r, u8 g, u8 b) {
        for (i32 oy = -1; oy <= 1; ++oy) {
            for (i32 ox = -1; ox <= 1; ++ox) {
                const i32 nx = static_cast<i32>(x) + ox;
                const i32 ny = static_cast<i32>(y) + oy;
                if (nx < 0 || ny < 0 || nx >= w || ny >= h) {
                    continue;
                }
                u8* px = rgb + tidx(w, static_cast<u16>(nx), static_cast<u16>(ny)) * 3u;
                px[0] = r;
                px[1] = g;
                px[2] = b;
            }
        }
    };
    paint(sx, sy, 255, 0, 0);
    paint(dx, dy, 0, 255, 0);
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t n = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, n, fp) == n;
    std::fclose(fp);
    return ok;
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
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    WhiteboardMng::init(w, h);
    u16 sx = 0;
    u16 sy = 0;
    u16 dx = 0;
    u16 dy = 0;
    if (!pick_src_dst(state, &sx, &sy, &dx, &dy)) {
        std::printf("*** FAILED pick_src_dst\n");
        WhiteboardMng::terminate();
        return 1;
    }
    std::printf("picked src (%u,%u) dst (%u,%u)\n",
        (unsigned)sx, (unsigned)sy, (unsigned)dx, (unsigned)dy);
    WalkP2P walk;
    if (!walk.ok()) {
        std::printf("*** FAILED WalkP2P checkout\n");
        WhiteboardMng::terminate();
        return 1;
    }
    u16 dmax = 0;
    const clock_t t0 = clock();
    const bool ok = GenerateDistanceP2P::generate(
        state, *g_rt_statics, sx, sy, dx, dy, walk, nullptr, &dmax);
    const double gen_sec = static_cast<double>(clock() - t0) / CLOCKS_PER_SEC;
    if (!ok) {
        std::printf("*** FAILED generate (%.6f s)\n", gen_sec);
        WhiteboardMng::terminate();
        return 1;
    }
    u8* path_m = new u8[state.m_map.tile_n()];
    EnterRec* ent = new EnterRec[k_enter_max];
    u32 steps = 0;
    u32 turns = 0;
    CostHist hist[k_cost_hist_n];
    u32 hist_n = 0;
    u32 ent_n = 0;
    const clock_t t1 = clock();
    const bool walked = sim_walk(
        walk, state, sx, sy, dx, dy, path_m, &steps, &turns, hist, &hist_n, ent, &ent_n);
    const double walk_sec = static_cast<double>(clock() - t1) / CLOCKS_PER_SEC;
    if (!walked) {
        std::printf("*** FAILED walk steps=%u turns=%u\n", steps, turns);
        delete[] ent;
        delete[] path_m;
        WhiteboardMng::terminate();
        return 1;
    }
    cost_sort(hist, hist_n);
    cost_print(hist, hist_n);
    if (!save_trace(G_TRACE, ent, ent_n)) {
        std::printf("*** FAILED save %s\n", G_TRACE);
        delete[] ent;
        delete[] path_m;
        WhiteboardMng::terminate();
        return 1;
    }
    u8* rgb = new u8[static_cast<size_t>(w) * static_cast<size_t>(h) * 3u];
    build_img(state, walk.turn(), path_m, w, h, sx, sy, dx, dy, dmax, rgb);
    if (!save_ppm(G_OUT, rgb, w, h)) {
        std::printf("*** FAILED save %s\n", G_OUT);
        delete[] rgb;
        delete[] ent;
        delete[] path_m;
        WhiteboardMng::terminate();
        return 1;
    }
    delete[] rgb;
    delete[] ent;
    delete[] path_m;
    std::printf("gen %.6f s walk %.6f s steps=%u turns=%u dmax=%u saved %s\n", gen_sec, walk_sec, steps, turns, (u32)dmax, G_OUT);
    std::printf("trace entries=%u saved %s\n", (u32)ent_n, G_TRACE);
    std::printf("*** PASSED walk_p2p\n");
    WhiteboardMng::terminate();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
