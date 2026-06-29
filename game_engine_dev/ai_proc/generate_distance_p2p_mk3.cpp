//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_p2p_mk3.h"
#include "game_map_defs.h"
#include "map_bit_array_overlay.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_none = 0xFFFFu;
static const u32 k_pred_none = 0xFFFFFFFFu;
static const u16 k_mp_z = PATH_MP_TURN;
static const u8 k_u4_sent = static_cast<u8>(Generate_DistanceP2P_Mk3::k_turn_sent);
static const u8 k_step_top = static_cast<u8>(Generate_DistanceP2P_Mk3::k_ovl_mod - 1u);
static const i16 k_dx4[] = {0, 1, 0, -1};
static const i16 k_dy4[] = {-1, 0, 1, 0};

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

static u8 step_from_parent (
    const u8* su4,
    const u16* turn,
    u32 pi,
    u32 ci,
    u16 tc) {
    if (pi == ci) {
        return k_step_top;
    }
    const u16 tp = turn[pi];
    if (tc > tp) {
        return k_step_top;
    }
    if (tc == tp) {
        if (su4[pi] == 0u) {
            return 0u;
        }
        return static_cast<u8>(su4[pi] - 1u);
    }
    return k_step_top;
}

static void wr_ovl_i (
    u8* tu4,
    u8* su4,
    const u16* turn,
    u32 ci,
    u32 pi,
    u16 macro_t) {
    tu4[ci] = static_cast<u8>(macro_t % Generate_DistanceP2P_Mk3::k_ovl_mod);
    su4[ci] = step_from_parent(su4, turn, pi, ci, macro_t);
}

static void flush_ovl (
    MapBitArrayOverlay* turn_o,
    MapBitArrayOverlay* step_o,
    const u8* tu4,
    const u8* su4) {
    turn_o->assign_u8(tu4);
    step_o->assign_u8(su4);
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
    u32* pred,
    u8* tu4,
    u8* su4,
    u16* out_max) {
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(ux) + k_dx4[k];
        const i32 ny = static_cast<i32>(uy) + k_dy4[k];
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
        pred[vi] = ui;
        wr_ovl_i(tu4, su4, turn, vi, ui, nt);
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
    u32* pred,
    u16* sq_spent,
    u8* tu4,
    u8* su4,
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
                    wr_ovl_i(tu4, su4, turn, si, pred[si], st);
                    q_nxt.push(sx, sy);
                    continue;
                }
                if (bud < k_min) {
                    mp[si] = k_mp_z;
                    wr_ovl_i(tu4, su4, turn, si, pred[si], st);
                    q_nxt.push(sx, sy);
                    continue;
                }
            }
            if (bud < k_min) {
                continue;
            }
            if (rem == 0) {
                mp[si] = mp_enc(k_mp);
                wr_ovl_i(tu4, su4, turn, si, pred[si], st);
            }
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, st, sq, q_nxt, turn, mp, pred, tu4, su4, out_max);
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
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, turn[si], sq, q_nxt, turn, mp, pred, tu4, su4, out_max);
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
//=> - Generate_DistanceP2P_Mk3 -
//================================================================================================================================

bool Generate_DistanceP2P_Mk3::generate (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    MapBitArrayOverlay* turn_o,
    MapBitArrayOverlay* step_o,
    u32* pred,
    u16** scr,
    u16* out_max) {
    if (terrain == nullptr || turn_o == nullptr || step_o == nullptr || pred == nullptr || scr == nullptr || out_max == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (turn_o->width() != w || turn_o->height() != h || turn_o->bits_per_val() != k_turn_bpv) {
        return false;
    }
    if (step_o->width() != w || step_o->height() != h || step_o->bits_per_val() != k_step_bpv) {
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
    u16* turn_f = new u16[tile_n];
    u8* tu4 = new u8[tile_n];
    u8* su4 = new u8[tile_n];
    u16* mp = scr[0];
    u16* sq_spent = scr[1];
    const i32 q_words = static_cast<i32>(tile_n);
    WB_QueXY q_cur(q_words);
    WB_QueXY q_nxt(q_words);
    WB_QueXY sq(q_words);
    if (!q_cur.ok() || !q_nxt.ok() || !sq.ok() || turn_f == nullptr || tu4 == nullptr || su4 == nullptr) {
        delete[] turn_f;
        delete[] tu4;
        delete[] su4;
        return false;
    }
    std::memset(tu4, k_u4_sent, static_cast<size_t>(tile_n));
    std::memset(su4, k_u4_sent, static_cast<size_t>(tile_n));
    for (u32 i = 0; i < tile_n; ++i) {
        turn_f[i] = k_turn_none;
        mp[i] = k_mp_z;
        pred[i] = k_pred_none;
    }
    turn_f[dst_i] = 0;
    mp[dst_i] = k_mp_z;
    pred[dst_i] = dst_i;
    wr_ovl_i(tu4, su4, turn_f, dst_i, dst_i, 0);
    q_cur.push(dst_x, dst_y);
    *out_max = 0;
    if (src_i == dst_i) {
        flush_ovl(turn_o, step_o, tu4, su4);
        delete[] turn_f;
        delete[] tu4;
        delete[] su4;
        return true;
    }
    const bool ok = flood_p2p(terrain, rivers, w, h, tile_n, src_i, turn_f, mp, pred, sq_spent, tu4, su4, q_cur, q_nxt, sq, out_max);
    if (ok) {
        flush_ovl(turn_o, step_o, tu4, su4);
    } else {
        turn_o->fill_all(k_turn_sent);
        step_o->fill_all(k_step_sent);
        for (u32 i = 0; i < tile_n; ++i) {
            pred[i] = k_pred_none;
        }
        *out_max = 0;
    }
    delete[] turn_f;
    delete[] tu4;
    delete[] su4;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
