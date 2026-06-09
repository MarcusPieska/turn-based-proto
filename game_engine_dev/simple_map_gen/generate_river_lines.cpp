//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_lines.h"

#include "generator_constants.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

enum RiverMoveDir : u8 {
    k_mv_l = 0,
    k_mv_r = 1,
    k_mv_u = 2,
    k_mv_d = 3,
};

struct RiverMoveEnt {
    u32 key;
    RiverMoveDir dir;
};

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static void mark_tile (u8* ov, u16 w, u16 h, i32 x, i32 y) {
    if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    ov[i] = 1;
}

static i32 abs_i32 (i32 v) {
    return (v < 0) ? -v : v;
}

static void build_moves (std::vector<RiverMoveDir>* moves, i32 dx, i32 dy) {
    moves->clear();
    moves->reserve(static_cast<size_t>(abs_i32(dx) + abs_i32(dy)));
    for (i32 i = 0; i < abs_i32(dx); ++i) {
        moves->push_back((dx > 0) ? k_mv_r : k_mv_l);
    }
    for (i32 i = 0; i < abs_i32(dy); ++i) {
        moves->push_back((dy > 0) ? k_mv_d : k_mv_u);
    }
}

static void inject_lateral_moves (std::vector<RiverMoveDir>* moves) {
    if (moves->empty()) {
        return;
    }
    u32 cnt[4] = {0, 0, 0, 0};
    for (RiverMoveDir m : *moves) {
        cnt[static_cast<u8>(m)]++;
    }
    const u32 total = static_cast<u32>(moves->size());
    u32 max_c = 0;
    RiverMoveDir dom = k_mv_l;
    for (u8 i = 0; i < 4; ++i) {
        if (cnt[i] > max_c) {
            max_c = cnt[i];
            dom = static_cast<RiverMoveDir>(i);
        }
    }
    if (max_c * 100u <= total * 70u) {
        return;
    }
    const u32 extra = total / 10u;
    if (extra == 0) {
        return;
    }
    if (dom == k_mv_d || dom == k_mv_u) {
        for (u32 i = 0; i < extra; ++i) {
            moves->push_back(k_mv_l);
        }
        for (u32 i = 0; i < extra; ++i) {
            moves->push_back(k_mv_r);
        }
    } else {
        for (u32 i = 0; i < extra; ++i) {
            moves->push_back(k_mv_u);
        }
        for (u32 i = 0; i < extra; ++i) {
            moves->push_back(k_mv_d);
        }
    }
}

static void exec_scrambled_moves (u8* ov, u16 w, u16 h, i32 x1, i32 y1, const std::vector<RiverMoveDir>& moves) {
    if (moves.empty()) {
        mark_tile(ov, w, h, x1, y1);
        return;
    }
    std::vector<RiverMoveEnt> lst;
    lst.reserve(moves.size());
    for (RiverMoveDir m : moves) {
        RiverMoveEnt e = {};
        e.key = static_cast<u32>(std::rand());
        e.dir = m;
        lst.push_back(e);
    }
    std::sort(lst.begin(), lst.end(), [](const RiverMoveEnt& a, const RiverMoveEnt& b) {
        return a.key < b.key;
    });
    i32 x = x1;
    i32 y = y1;
    mark_tile(ov, w, h, x, y);
    for (const RiverMoveEnt& e : lst) {
        if (e.dir == k_mv_l) {
            x--;
        } else if (e.dir == k_mv_r) {
            x++;
        } else if (e.dir == k_mv_u) {
            y--;
        } else {
            y++;
        }
        mark_tile(ov, w, h, x, y);
    }
}

static void mark_scrambled_path (u8* ov, u16 w, u16 h, i32 x1, i32 y1, i32 x2, i32 y2) {
    std::vector<RiverMoveDir> moves;
    build_moves(&moves, x2 - x1, y2 - y1);
    inject_lateral_moves(&moves);
    exec_scrambled_moves(ov, w, h, x1, y1, moves);
}

static bool find_nearest_water (
    const u8* terrain,
    u16 w,
    u16 h,
    i32 sx,
    i32 sy,
    i32* out_wx,
    i32* out_wy) 
{
    if (terrain == nullptr || out_wx == nullptr || out_wy == nullptr) {
        return false;
    }
    if (sx < 0 || sy < 0 || static_cast<u32>(sx) >= static_cast<u32>(w) || static_cast<u32>(sy) >= static_cast<u32>(h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::vector<u8> vis(n, 0);
    std::vector<i32> qx;
    std::vector<i32> qy;
    std::vector<i32> qd;
    const u32 si = static_cast<u32>(sy) * static_cast<u32>(w) + static_cast<u32>(sx);
    vis[si] = 1;
    qx.push_back(sx);
    qy.push_back(sy);
    qd.push_back(0);
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    i32 best_wx = -1;
    i32 best_wy = -1;
    i32 best_d = -1;
    for (std::size_t qi = 0; qi < qx.size(); ++qi) {
        const i32 cx = qx[qi];
        const i32 cy = qy[qi];
        const i32 cd = qd[qi];
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = cx + dx4[d];
            const i32 ny = cy + dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (is_water(terrain[ni])) {
                if (best_d == -1 || cd + 1 < best_d) {
                    best_d = cd + 1;
                    best_wx = nx;
                    best_wy = ny;
                }
                continue;
            }
            if (vis[ni]) {
                continue;
            }
            vis[ni] = 1;
            qx.push_back(nx);
            qy.push_back(ny);
            qd.push_back(cd + 1);
        }
        if (best_d != -1 && cd > best_d) {
            break;
        }
    }
    if (best_d == -1) {
        return false;
    }
    *out_wx = best_wx;
    *out_wy = best_wy;
    return true;
}

//================================================================================================================================
//=> - Generate_RiverLines -
//================================================================================================================================

RiverLinesResult* Generate_RiverLines::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const RiverSectorsResult* sectors,
    const RiverNetworkResult* network,
    u32 seed) 
{
    if (terrain == nullptr || w == 0 || h == 0 || sectors == nullptr || sectors->nodes == nullptr || network == nullptr) {
        return nullptr;
    }
    if (network->conn_n > 0 && network->conns == nullptr) {
        return nullptr;
    }
    if (network->coastal == nullptr || network->river_sys == nullptr) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    RiverLinesResult* out = new RiverLinesResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->overlay = new u8[n];
    if (out->overlay == nullptr) {
        delete out;
        return nullptr;
    }
    std::memset(out->overlay, 0, n * sizeof(u8));
    std::srand(seed);
    for (u16 ci = 0; ci < network->conn_n; ++ci) {
        const u16 ia = network->conns[ci].a;
        const u16 ib = network->conns[ci].b;
        if (ia >= sectors->sector_n || ib >= sectors->sector_n) {
            continue;
        }
        const i32 x1 = static_cast<i32>(sectors->nodes[ia].cx);
        const i32 y1 = static_cast<i32>(sectors->nodes[ia].cy);
        const i32 x2 = static_cast<i32>(sectors->nodes[ib].cx);
        const i32 y2 = static_cast<i32>(sectors->nodes[ib].cy);
        mark_scrambled_path(out->overlay, w, h, x1, y1, x2, y2);
    }
    for (u32 si = 0; si < static_cast<u32>(network->sector_n); ++si) {
        if (!network->coastal[si]) {
            continue;
        }
        if (network->river_sys[si] == static_cast<u16>(RIVER_SYS_NONE)) {
            continue;
        }
        const i32 x1 = static_cast<i32>(sectors->nodes[si].cx);
        const i32 y1 = static_cast<i32>(sectors->nodes[si].cy);
        i32 wx = 0;
        i32 wy = 0;
        if (!find_nearest_water(terrain, w, h, x1, y1, &wx, &wy)) {
            continue;
        }
        mark_scrambled_path(out->overlay, w, h, x1, y1, wx, wy);
    }
    return out;
}

void Generate_RiverLines::free_result (RiverLinesResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->overlay;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
