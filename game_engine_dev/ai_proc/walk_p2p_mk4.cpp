//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_p2p_mk4.h"
#include "generate_distance_p2p_mk4.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_dist_sent = Generate_DistanceP2P_Mk4::k_dist_sent;
static const i16 k_dx4[] = {0, 1, 0, -1};
static const i16 k_dy4[] = {-1, 0, 1, 0};

//================================================================================================================================
//=> - WalkP2P_Mk4 -
//================================================================================================================================

u32 WalkP2P_Mk4::tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

u16 WalkP2P_Mk4::move_cost (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 ux,
    u16 uy,
    u16 vx,
    u16 vy) {
    const u32 ui = tidx(w, ux, uy);
    const u32 vi = tidx(w, vx, vy);
    if (rivers != nullptr && rivers[ui] != 0u && rivers[vi] != 0u) {
        return PATH_COST_RIV;
    }
    if (terrain[vi] == TERR_HILLS[0]) {
        return PATH_COST_HILL;
    }
    return PATH_COST_STEP;
}

bool WalkP2P_Mk4::ovl_reach (const u16* dist, u16 w, u16 x, u16 y) {
    return dist[tidx(w, x, y)] != k_dist_sent;
}

bool WalkP2P_Mk4::find_lo_adj (
    u32 d,
    const u16* dist,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16& ox,
    u16& oy) {
    u32 lo_nd = 0xFFFFFFFFu;
    u16 lx = x;
    u16 ly = y;
    bool have = false;
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        if (!ovl_reach(dist, w, tx, ty)) {
            continue;
        }
        const u32 nd = static_cast<u32>(dist[tidx(w, tx, ty)]);
        if (nd >= d) {
            continue;
        }
        if (!have || nd < lo_nd) {
            lo_nd = nd;
            lx = tx;
            ly = ty;
            have = true;
        }
    }
    if (!have) {
        return false;
    }
    ox = lx;
    oy = ly;
    return true;
}

WalkP2P_Mk4::StepRes WalkP2P_Mk4::peek_step (
    const u16* dist,
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 x,
    u16 y) const {
    StepRes res = {};
    const u32 d = static_cast<u32>(dist[tidx(w, x, y)]);
    if (d >= static_cast<u32>(k_dist_sent)) {
        return res;
    }
    u16 tx = x;
    u16 ty = y;
    if (!find_lo_adj(d, dist, w, h, x, y, tx, ty)) {
        return res;
    }
    res.nx = tx;
    res.ny = ty;
    res.cost = move_cost(terrain, rivers, w, x, y, tx, ty);
    res.have = true;
    return res;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
