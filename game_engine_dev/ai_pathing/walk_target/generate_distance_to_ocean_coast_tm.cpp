//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_to_ocean_coast_tm.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "wb_que_xy.h"
#include <cstdio>
#include <ctime>

static const bool k_phase_time = true;

static double clk_sec (clock_t t0, clock_t t1) {
    return static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
}

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_none = 0xFFFFu;
static const u16 k_mp_z = PATH_MP_TURN;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_ocean (u8 t) {
    return t == TERR_OCEAN[0];
}

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

static void seed_coast (
    const u8* terrain,
    u16 w,
    u16 x,
    u16 y,
    u16* turn,
    u16* mp,
    WB_QueXY& coast_q) {
    const u32 i = tidx(w, x, y);
    if (!is_walk(terrain[i])) {
        return;
    }
    if (turn[i] != k_turn_none) {
        return;
    }
    turn[i] = 0;
    mp[i] = k_mp_z;
    coast_q.push(x, y);
}

static void seed_coast_nbrs (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 px,
    u16 py,
    u16* turn,
    u16* mp,
    WB_QueXY& coast_q) {
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
        const i32 nx = static_cast<i32>(px) + MAP_NBR4_DX[k];
        const i32 ny = static_cast<i32>(py) + MAP_NBR4_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h) {
            continue;
        }
        seed_coast(terrain, w, cx, cy, turn, mp, coast_q);
    }
}

static bool wtr_bords_land (
    const u8* terrain,
    u16 w,
    u16 h,
    u16 px,
    u16 py) {
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
        const i32 nx = static_cast<i32>(px) + MAP_NBR4_DX[k];
        const i32 ny = static_cast<i32>(py) + MAP_NBR4_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h) {
            continue;
        }
        if (is_walk(terrain[tidx(w, cx, cy)])) {
            return true;
        }
    }
    return false;
}

static void flood_ocean_wtr (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 tile_n,
    u16* oc,
    u16* turn,
    u16* mp,
    WB_QueXY& wq,
    WB_QueXY& wq_nxt,
    WB_QueXY& coast_q) {
    for (u32 i = 0; i < tile_n; ++i) {
        oc[i] = 0;
    }
    wq.clear();
    wq_nxt.clear();
    coast_q.clear();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (!is_ocean(terrain[i]) || oc[i] != 0) {
                continue;
            }
            wq.clear();
            wq.push(x, y);
            oc[i] = 1;
            while (wq.count() > 0u) {
                for (u32 qh = 0; qh < wq.count(); ++qh) {
                    const u16 px = wq.x_at(qh);
                    const u16 py = wq.y_at(qh);
                    if (wtr_bords_land(terrain, w, h, px, py)) {
                        seed_coast_nbrs(terrain, w, h, px, py, turn, mp, coast_q);
                    }
                    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
                        const i32 nx = static_cast<i32>(px) + MAP_NBR4_DX[k];
                        const i32 ny = static_cast<i32>(py) + MAP_NBR4_DY[k];
                        if (nx < 0 || ny < 0) {
                            continue;
                        }
                        const u16 cx = static_cast<u16>(nx);
                        const u16 cy = static_cast<u16>(ny);
                        if (cx >= w || cy >= h) {
                            continue;
                        }
                        const u32 ni = tidx(w, cx, cy);
                        if (!is_water(terrain[ni]) || oc[ni] != 0) {
                            continue;
                        }
                        oc[ni] = 1;
                        wq_nxt.push(cx, cy);
                        if (wtr_bords_land(terrain, w, h, cx, cy)) {
                            seed_coast_nbrs(terrain, w, h, cx, cy, turn, mp, coast_q);
                        }
                    }
                }
                wq.swap(wq_nxt);
                wq_nxt.clear();
            }
        }
    }
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

static void flood_coast_tm (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u32 tile_n,
    u16* turn,
    u16* mp,
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
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, st, sq, q_nxt, turn, mp, out_max);
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
            try_fan(terrain, rivers, w, h, si, sx, sy, bud, turn[si], sq, q_nxt, turn, mp, out_max);
            sq_spent[si] = mp_enc(bud);
        }
        q_cur.swap(q_nxt);
        q_nxt.clear();
    }
}

//================================================================================================================================
//=> - Generate_DistanceToOceanCoast_Tm -
//================================================================================================================================

bool Generate_DistanceToOceanCoast_Tm::generate (
    const u8* terrain,
    const u8* rivers,
    u16 w,
    u16 h,
    u16* turn,
    u16** scr,
    u16* out_max) {
    if (terrain == nullptr || turn == nullptr || scr == nullptr || out_max == nullptr || w == 0 || h == 0) {
        return false;
    }
    for (u32 s = 0; s < k_scr_n; ++s) {
        if (scr[s] == nullptr) {
            return false;
        }
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 q_words = static_cast<i32>(tile_n);
    u16* mp = scr[0];
    u16* oc = scr[1];
    WB_QueXY wq(q_words);
    WB_QueXY coast_q(q_words);
    WB_QueXY q_nxt(q_words);
    WB_QueXY sq(q_words);
    if (!wq.ok() || !coast_q.ok() || !q_nxt.ok() || !sq.ok()) {
        return false;
    }
    const clock_t t_all = k_phase_time ? clock() : 0;
    clock_t t0 = t_all;
    for (u32 i = 0; i < tile_n; ++i) {
        turn[i] = k_turn_none;
        mp[i] = k_mp_z;
    }
    if (k_phase_time) {
        t0 = clock();
    }
    flood_ocean_wtr(terrain, w, h, tile_n, oc, turn, mp, wq, q_nxt, coast_q);
    const clock_t t_ocean = k_phase_time ? clock() : 0;
    flood_coast_tm(terrain, rivers, w, h, tile_n, turn, mp, oc, coast_q, q_nxt, sq, out_max);
    if (k_phase_time) {
        const clock_t t_end = clock();
        std::printf("tm phase init:  %.6f s\n", clk_sec(t_all, t0));
        std::printf("tm phase ocean:  %.6f s\n", clk_sec(t0, t_ocean));
        std::printf("tm phase coast:  %.6f s\n", clk_sec(t_ocean, t_end));
        std::printf("tm phase total:  %.6f s\n", clk_sec(t_all, t_end));
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
