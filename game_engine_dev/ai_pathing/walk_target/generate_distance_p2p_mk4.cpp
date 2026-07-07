//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_p2p_mk4.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "runtime_trace_dbg.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Rules -
//================================================================================================================================
// 
// - Maintain queue of tiles that have been entered that still have open neighbors.
// - Process this queue once per turn; increment MP with lowest cost entry.
// - If move can be made, add target tile to queue, set MP to previous tiles budget minus entry cost.
// - If there are no open neighbors, the tile is removed from the queue.
// - Mark the tiles value when entering (macro turn of entry).
// - Enter only unmarked tiles.
// - Enter only when the cost is less than or equal to the budget.
//
//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_none = 0xFFFFu;
static const u32 k_pred_none = 0xFFFFFFFFu;
static const u16 k_dist_sent = Generate_DistanceP2P_Mk4::k_dist_sent;
static const i32 k_tile_bud = static_cast<i32>(PATH_COST_RIV);

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_walk (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_water(t);
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool ovl_marked (const u16* dist_u16, u32 ci) {
    return dist_u16[ci] != k_dist_sent;
}

static void wr_dist_enter (
    u16* dist_u16,
    u32 ci,
    u32 dst_i,
    u16 enter_v) {
    if (ci == dst_i) {
        return;
    }
    dist_u16[ci] = enter_v;
}

static u16 step_cost_i (const u8* terrain, const u8* rivers, u32 ui, u32 vi) {
    if (rivers != nullptr && rivers[ui] != 0u && rivers[vi] != 0u) {
        return PATH_COST_RIV;
    }
    if (terrain[vi] == TERR_HILLS[0]) {
        return PATH_COST_HILL;
    }
    return PATH_COST_STEP;
}

static bool has_open_nbr (
    const u8* terrain,
    const u16* dist_u16,
    u16 w,
    u16 h,
    u16 x,
    u16 y) {
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
        if (!is_walk(terrain[tidx(w, tx, ty)])) {
            continue;
        }
        if (!ovl_marked(dist_u16, tidx(w, tx, ty))) {
            return true;
        }
    }
    return false;
}

static const u16 k_que_on = 1u;
static const u16 k_ent_now = 2u;

static void que_push_open (
    WB_QueXY& q,
    u16* in_q,
    u16 w,
    u16 x,
    u16 y) {
    const u32 i = tidx(w, x, y);
    if ((in_q[i] & k_que_on) != 0u) {
        return;
    }
    if (!q.push(x, y)) {
        return;
    }
    in_q[i] |= k_que_on;
}

static void fan_tile (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u32 dst_i,
    u32 ui,
    u16 ux,
    u16 uy,
    u16 macro,
    u16* turn,
    u16* mp,
    u32* pred,
    u16* dist_u16,
    u16* in_nxt,
    WB_QueXY& q_nxt,
    u16* out_max) {
    mp[ui] = static_cast<u16>(static_cast<i32>(mp[ui]) + k_tile_bud);
    const i32 bud = static_cast<i32>(mp[ui]);
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
        const i32 nx = static_cast<i32>(ux) + MAP_NBR4_DX[k];
        const i32 ny = static_cast<i32>(uy) + MAP_NBR4_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 vx = static_cast<u16>(nx);
        const u16 vy = static_cast<u16>(ny);
        if (vx >= w || vy >= h) {
            continue;
        }
        const u32 vi = tidx(w, vx, vy);
        if (!is_walk(terrain[vi])) {
            continue;
        }
        if (ovl_marked(dist_u16, vi)) {
            TRACE_P2P_MK4_BLOCK((vx, vy, dist_u16[vi]));
            continue;
        }
        const u16 cost = step_cost_i(terrain, rivers, ui, vi);
        if (bud < static_cast<i32>(cost)) {
            continue;
        }
        turn[vi] = macro;
        mp[vi] = static_cast<u16>(bud - static_cast<i32>(cost));
        pred[vi] = ui;
        wr_dist_enter(dist_u16, vi, dst_i, macro);
        in_nxt[vi] |= k_ent_now;
        TRACE_P2P_MK4_ENTER((ux, uy, vx, vy, turn[ui], macro));
        if (macro > *out_max) {
            *out_max = macro;
        }
        if (has_open_nbr(terrain, dist_u16, w, h, vx, vy)) {
            que_push_open(q_nxt, in_nxt, w, vx, vy);
        }
    }
    if (has_open_nbr(terrain, dist_u16, w, h, ux, uy)) {
        que_push_open(q_nxt, in_nxt, w, ux, uy);
    }
}

static bool flood_p2p (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u32 tile_n,
    u32 dst_i,
    u32 src_i,
    u16* turn,
    u16* mp,
    u32* pred,
    u16* in_nxt,
    u16* dist_u16,
    WB_QueXY& q_cur,
    WB_QueXY& q_nxt,
    u16* out_max) {
    *out_max = 0;
    u16 macro = 0;
    while (q_cur.count() > 0u) {
        macro++;
        TRACE_NEW_TURN((macro));
        std::memset(in_nxt, 0, static_cast<size_t>(tile_n) * sizeof(u16));
        for (u32 qh = 0; qh < q_cur.count(); ++qh) {
            const u16 sx = q_cur.x_at(qh);
            const u16 sy = q_cur.y_at(qh);
            const u32 si = tidx(w, sx, sy);
            if ((in_nxt[si] & k_ent_now) != 0u) {
                TRACE_P2P_MK4_SKIP((sx, sy, turn[si]));
                continue;
            }
            fan_tile(terrain, rivers, w, h, dst_i, si, sx, sy, macro, turn, mp, pred, dist_u16, in_nxt, q_nxt, out_max);
        }
        q_cur.swap(q_nxt);
        q_nxt.clear();
        if (ovl_marked(dist_u16, src_i)) {
            return true;
        }
    }
    return ovl_marked(dist_u16, src_i);
}

//================================================================================================================================
//=> - Generate_DistanceP2P_Mk4 -
//================================================================================================================================

bool Generate_DistanceP2P_Mk4::generate (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u16* dist_o,
    u32* pred,
    u16** scr,
    u16* out_max) {
    if (w == 0 || h == 0) {
        return false;
    }
    if (terrain == nullptr || dist_o == nullptr || pred == nullptr || scr == nullptr || out_max == nullptr) {
        return false;
    }
    if (src_x >= w || src_y >= h || dst_x >= w || dst_y >= h) {
        return false;
    }
    for (u32 s = 0; s < k_scr_n; ++s) {
        if (scr[s] == nullptr) {
            return false;
        }
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 src_i = tidx(w, src_x, src_y);
    const u32 dst_i = tidx(w, dst_x, dst_y);
    if (!is_walk(terrain[src_i]) || !is_walk(terrain[dst_i])) {
        return false;
    }
    u16* mp = scr[0];
    u16* in_nxt = scr[1];
    u16* turn_f = scr[2];
    const i32 q_words = static_cast<i32>(tile_n);
    WB_QueXY q_cur(q_words);
    WB_QueXY q_nxt(q_words);
    if (!q_cur.ok() || !q_nxt.ok()) {
        return false;
    }
    std::memset(dist_o, 0xFF, static_cast<size_t>(tile_n) * sizeof(u16));
    for (u32 i = 0; i < tile_n; ++i) {
        turn_f[i] = k_turn_none;
        mp[i] = 0;
        pred[i] = k_pred_none;
    }
    turn_f[dst_i] = 0;
    mp[dst_i] = 0;
    pred[dst_i] = dst_i;
    dist_o[dst_i] = 0u;
    q_cur.push(dst_x, dst_y);
    *out_max = 0;
    if (src_i == dst_i) {
        return true;
    }
    const bool ok = flood_p2p(terrain, rivers, w, h, tile_n, dst_i, src_i, turn_f, mp, pred, in_nxt, dist_o, q_cur, q_nxt, out_max);
    if (!ok) {
        std::memset(dist_o, 0xFF, static_cast<size_t>(tile_n) * sizeof(u16));
        for (u32 i = 0; i < tile_n; ++i) {
            pred[i] = k_pred_none;
        }
        *out_max = 0;
    }
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
