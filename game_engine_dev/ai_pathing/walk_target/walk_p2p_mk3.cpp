//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_p2p_mk3.h"
#include "generate_distance_p2p_mk3.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "map_bit_array_overlay.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_pred_none = 0xFFFFFFFFu;
static const u32 k_ovl_mod = Generate_DistanceP2P_Mk3::k_ovl_mod;

//================================================================================================================================
//=> - WalkP2P_Mk3 -
//================================================================================================================================

u32 WalkP2P_Mk3::tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

u16 WalkP2P_Mk3::move_cost (
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

bool WalkP2P_Mk3::ovl_reach (const u32* pred, u16 w, u16 x, u16 y) {
    return pred[tidx(w, x, y)] != k_pred_none;
}

bool WalkP2P_Mk3::ovl_dst (const u32* pred, u16 w, u16 x, u16 y) {
    const u32 i = tidx(w, x, y);
    return pred[i] == i;
}

bool WalkP2P_Mk3::step_cl_u4 (u32 s, u32 ns) {
    return ns > s;
}

bool WalkP2P_Mk3::closer_u4 (u32 t, u32 s, u32 nt, u32 ns) {
    const u32 td = (t + k_ovl_mod - nt) % k_ovl_mod;
    if (td >= 1u && td <= (k_ovl_mod / 2u)) {
        return true;
    }
    if (nt != t) {
        return false;
    }
    return step_cl_u4(s, ns);
}

bool WalkP2P_Mk3::pick_nbr (
    const MapBitArrayOverlay& turn_o,
    const MapBitArrayOverlay& step_o,
    const u32* pred,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16& ox,
    u16& oy) {
    if (!ovl_reach(pred, w, x, y)) {
        return false;
    }
    const u32 t = turn_o.get(x, y);
    const u32 s = step_o.get(x, y);
    if (ovl_dst(pred, w, x, y)) {
        return false;
    }
    u32 bt = t;
    u32 bs = s;
    u16 bx = x;
    u16 by = y;
    bool found = false;
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
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
        if (!ovl_reach(pred, w, tx, ty)) {
            continue;
        }
        const u32 nt = turn_o.get(tx, ty);
        const u32 ns = step_o.get(tx, ty);
        if (!closer_u4(t, s, nt, ns)) {
            continue;
        }
        if (!found) {
            bt = nt;
            bs = ns;
            bx = tx;
            by = ty;
            found = true;
            continue;
        }
        const u32 ntd = (t + k_ovl_mod - nt) % k_ovl_mod;
        const bool n_turn = ntd >= 1u && ntd <= (k_ovl_mod / 2u);
        const u32 btd = (t + k_ovl_mod - bt) % k_ovl_mod;
        const bool b_turn = btd >= 1u && btd <= (k_ovl_mod / 2u);
        if (n_turn && !b_turn) {
            bt = nt;
            bs = ns;
            bx = tx;
            by = ty;
            continue;
        }
        if (n_turn == b_turn && nt == t && ns > bs) {
            bt = nt;
            bs = ns;
            bx = tx;
            by = ty;
        }
    }
    if (!found) {
        return false;
    }
    ox = bx;
    oy = by;
    return true;
}

WalkP2P_Mk3::StepRes WalkP2P_Mk3::peek_step (
    const MapBitArrayOverlay& turn_o,
    const MapBitArrayOverlay& step_o,
    const u32* pred,
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 x,
    u16 y) const {
    StepRes res = {};
    u16 tx = x;
    u16 ty = y;
    if (!pick_nbr(turn_o, step_o, pred, w, h, x, y, tx, ty)) {
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
