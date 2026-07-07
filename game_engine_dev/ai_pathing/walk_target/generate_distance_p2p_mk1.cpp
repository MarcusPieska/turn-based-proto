//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_p2p_mk1.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_none = 0xFFFFu;
static const u32 k_pred_none = 0xFFFFFFFFu;
static const u16 k_mp_z = PATH_MP_TURN;

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

static i32 mp_dec (u16 v) {
    return static_cast<i32>(v) - static_cast<i32>(k_mp_z);
}

static u16 mp_enc (i32 rem) {
    return static_cast<u16>(rem + static_cast<i32>(k_mp_z));
}

static bool arr_better (
    u16 old_t,
    u16 old_mp,
    u16 new_t,
    i32 new_r) {
    if (old_t == k_turn_none) {
        return true;
    }
    if (new_t < old_t) {
        return true;
    }
    if (new_t > old_t) {
        return false;
    }
    return new_r > mp_dec(old_mp);
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

static void try_fan (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u32 ui,
    u16 ux,
    u16 uy,
    i32 bud,
    u16 ct,
    WB_QueXY& sq,
    WB_QueXY& q_nxt,
    u16* turn,
    u16* mp,
    u16* step,
    u32* pred,
    u16* out_max) {
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
        const u16 cost = step_cost_i(terrain, rivers, ui, vi);
        const i32 nrem = bud - static_cast<i32>(cost);
        const u16 nt = nrem <= 0
            ? static_cast<u16>(ct + 1u) : ct;
        if (!arr_better(turn[vi], mp[vi], nt, nrem)) {
            continue;
        }
        turn[vi] = nt;
        mp[vi] = mp_enc(nrem);
        step[vi] = mp[vi];
        pred[vi] = ui;
        if (nt > *out_max) {
            *out_max = nt;
        }
        if (nrem <= 0) {
            q_nxt.push(vx, vy);
        } else {
            sq.push(vx, vy);
        }
    }
}

static bool flood_p2p (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u32 tile_n,
    u32 src_i,
    u16* turn,
    u16* mp,
    u16* step,
    u32* pred,
    u16* sq_spent,
    WB_QueXY& q_cur,
    WB_QueXY& q_nxt,
    WB_QueXY& sq,
    u16* out_max) {
    *out_max = 0;
    const i32 k_mp = static_cast<i32>(PATH_MP_TURN);
    const i32 k_min = static_cast<i32>(PATH_COST_STEP);
    while (q_cur.count() > 0u) {
        for (u32 i = 0; i < tile_n; ++i) {
            sq_spent[i] = 0;
        }
        sq.clear();
        for (u32 qh = 0; qh < q_cur.count(); ++qh) {
            const u16 sx = q_cur.x_at(qh);
            const u16 sy = q_cur.y_at(qh);
            const u32 si = tidx(w, sx, sy);
            const u16 st = turn[si];
            if (st >= 65534u) {
                continue;
            }
            const i32 rem = mp_dec(mp[si]);
            if (rem > 0) {
                sq.push(sx, sy);
                continue;
            }
            const i32 bud = rem + k_mp;
            if (rem < 0) {
                if (bud <= 0) {
                    mp[si] = mp_enc(bud);
                    q_nxt.push(sx, sy);
                    continue;
                }
                if (bud < k_min) {
                    mp[si] = k_mp_z;
                    q_nxt.push(sx, sy);
                    continue;
                }
            }
            if (bud < k_min) {
                continue;
            }
            if (rem == 0) {
                mp[si] = mp_enc(k_mp);
                step[si] = mp[si];
            }
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, st, sq, q_nxt, turn, mp, step, pred, out_max);
        }
        for (u32 sh = 0; sh < sq.count(); ++sh) {
            const u16 sx = sq.x_at(sh);
            const u16 sy = sq.y_at(sh);
            const u32 si = tidx(w, sx, sy);
            const i32 bud = mp_dec(mp[si]);
            if (bud <= 0) {
                continue;
            }
            if (sq_spent[si] != 0) {
                const i32 spent = mp_dec(sq_spent[si]);
                if (bud <= spent) {
                    continue;
                }
            }
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, turn[si], sq, q_nxt, turn, mp, step, pred, out_max);
            sq_spent[si] = mp_enc(bud);
        }
        q_cur.swap(q_nxt);
        q_nxt.clear();
        if (turn[src_i] != k_turn_none) {
            return true;
        }
    }
    return turn[src_i] != k_turn_none;
}

//================================================================================================================================
//=> - Generate_DistanceP2P_Mk1 -
//================================================================================================================================

bool Generate_DistanceP2P_Mk1::generate (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    u16* turn,
    u16* step,
    u32* pred,
    u16** scr,
    u16* out_max) {
    if (terrain == nullptr || turn == nullptr || step == nullptr || pred == nullptr || scr == nullptr || out_max == nullptr || w == 0 || h == 0) {
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
    u16* sq_spent = scr[1];
    const i32 q_words = static_cast<i32>(tile_n);
    WB_QueXY q_cur(q_words);
    WB_QueXY q_nxt(q_words);
    WB_QueXY sq(q_words);
    if (!q_cur.ok() || !q_nxt.ok() || !sq.ok()) {
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        turn[i] = k_turn_none;
        step[i] = k_mp_z;
        mp[i] = k_mp_z;
        pred[i] = k_pred_none;
    }
    turn[dst_i] = 0;
    step[dst_i] = k_mp_z;
    mp[dst_i] = k_mp_z;
    pred[dst_i] = dst_i;
    q_cur.push(dst_x, dst_y);
    *out_max = 0;
    if (src_i == dst_i) {
        return true;
    }
    const bool ok = flood_p2p(terrain, rivers, w, h, tile_n, src_i, turn, mp, step, pred, sq_spent, q_cur, q_nxt, sq, out_max);
    if (!ok) {
        for (u32 i = 0; i < tile_n; ++i) {
            turn[i] = k_turn_none;
            step[i] = k_mp_z;
            pred[i] = k_pred_none;
        }
        *out_max = 0;
    }
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
