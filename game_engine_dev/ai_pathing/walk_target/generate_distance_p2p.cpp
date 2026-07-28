//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_p2p.h"

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_movement_mng.h"
#include "walk_p2p.h"
#include "whiteboard_mng.h"

#include <cstring>

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_sent = GenerateDistanceP2P::k_turn_sent;
static const u32 k_nil = 0xFFFFFFFFu;
static const u32 k_bkt_n = 4096u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_wtr (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]
        || t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0];
}

static bool is_land (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_wtr(t);
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static u16 idx_x (u32 i, u16 w) {
    return static_cast<u16>(i % static_cast<u32>(w));
}

static u16 idx_y (u32 i, u16 w) {
    return static_cast<u16>(i / static_cast<u32>(w));
}

static bool can_tile (const GameState& s, u16 x, u16 y, const Whiteboard_1B* mask) {
    const u8 t = s.m_map.get_terrain(x, y);
    if (!is_land(t) || TileAttrTables::terr(t).mvt_cost == 0u) {
        return false;
    }
    if (mask != nullptr && mask->rd(x, y) == 0u) {
        return false;
    }
    return true;
}

static i32 rem_dec (u16 v) {
    return static_cast<i32>(static_cast<i16>(v));
}

static u16 rem_enc (i32 rem) {
    return static_cast<u16>(static_cast<i16>(rem));
}

static i32 min_edge () {
    u16 m = 0xFFFFu;
    auto take = [&](u16 c) {
        if (c > 0u && c < m) {
            m = c;
        }
    };
    take(TileAttrTables::riv().mvt_cost);
    for (u16 i = 0; i < TileAttrTables::terr_n(); ++i) {
        take(TileAttrTables::terr(static_cast<u8>(i)).mvt_cost);
    }
    for (u16 i = 0; i < TileAttrTables::road_n(); ++i) {
        take(TileAttrTables::road(static_cast<u8>(i)).mvt_cost);
    }
    if (m == 0xFFFFu) {
        return 0;
    }
    return static_cast<i32>(m);
}

static bool better (u16 old_t, u16 old_rem, u16 nt, i32 nr) {
    if (old_t == k_turn_sent) {
        return true;
    }
    if (nt < old_t) {
        return true;
    }
    if (nt > old_t) {
        return false;
    }
    return nr > rem_dec(old_rem);
}

struct Nbr {
    u16 x;
    u16 y;
    u32 i;
    i32 c;
};

struct BktQ {
    u32* heads;
    u32* nxt;
    u32* prv;
    u8* in_q;
    u16* q_turn;
    u16* q_rem;
    u16 min_t;
    u32 open_n;
};

static void bkt_init (BktQ& b, u32* heads, u32* nxt, u32* prv, u8* in_q, u16* q_turn, u16* q_rem, u32 tile_n) {
    b.heads = heads;
    b.nxt = nxt;
    b.prv = prv;
    b.in_q = in_q;
    b.q_turn = q_turn;
    b.q_rem = q_rem;
    b.min_t = 0u;
    b.open_n = 0u;
    for (u32 t = 0; t < k_bkt_n; ++t) {
        heads[t] = k_nil;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        nxt[i] = k_nil;
        prv[i] = k_nil;
        in_q[i] = 0u;
    }
}

static void bkt_unlink (BktQ& b, u32 vi) {
    const u16 t = b.q_turn[vi];
    if (t >= k_bkt_n) {
        return;
    }
    const u32 p = b.prv[vi];
    const u32 n = b.nxt[vi];
    if (p != k_nil) {
        b.nxt[p] = n;
    } else {
        b.heads[t] = n;
    }
    if (n != k_nil) {
        b.prv[n] = p;
    }
    b.nxt[vi] = k_nil;
    b.prv[vi] = k_nil;
}

static bool bkt_push (BktQ& b, u32 vi, u16 at, u16 rem_v) {
    if (at >= k_bkt_n) {
        return false;
    }
    if (b.in_q[vi] != 0u) {
        if (b.q_turn[vi] == at) {
            b.q_rem[vi] = rem_v;
            return true;
        }
        bkt_unlink(b, vi);
        b.open_n--;
        b.in_q[vi] = 0u;
    }
    const u32 n = b.heads[at];
    b.nxt[vi] = n;
    b.prv[vi] = k_nil;
    if (n != k_nil) {
        b.prv[n] = vi;
    }
    b.heads[at] = vi;
    b.in_q[vi] = 1u;
    b.q_turn[vi] = at;
    b.q_rem[vi] = rem_v;
    b.open_n++;
    if (at < b.min_t) {
        b.min_t = at;
    }
    return true;
}

static u32 bkt_pop (BktQ& b) {
    while (b.min_t < k_bkt_n && b.heads[b.min_t] == k_nil) {
        b.min_t++;
    }
    if (b.min_t >= k_bkt_n || b.heads[b.min_t] == k_nil) {
        return k_nil;
    }
    const u32 vi = b.heads[b.min_t];
    bkt_unlink(b, vi);
    b.in_q[vi] = 0u;
    b.open_n--;
    return vi;
}

static u32 gather_nbrs (
    const GameState& s,
    u16 w,
    u16 h,
    u16 ux,
    u16 uy,
    const Whiteboard_1B* mask,
    Nbr* out) {
    u32 n = 0u;
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 nx = static_cast<i32>(ux) + MAP_NBR8_DX[k];
        const i32 ny = static_cast<i32>(uy) + MAP_NBR8_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 vx = static_cast<u16>(nx);
        const u16 vy = static_cast<u16>(ny);
        if (vx >= w || vy >= h) {
            continue;
        }
        if (!can_tile(s, vx, vy, mask)) {
            continue;
        }
        const i16 cost = UnitMovementMng::tile_cost(s, ux, uy, vx, vy);
        if (cost <= 0) {
            continue;
        }
        out[n].x = vx;
        out[n].y = vy;
        out[n].i = tidx(w, vx, vy);
        out[n].c = static_cast<i32>(cost);
        n++;
    }
    return n;
}

static bool any_offer_possible (u16 ut, const Nbr* nbrs, u32 nn, const u16* turn) {
    for (u32 i = 0; i < nn; ++i) {
        const u16 vt = turn[nbrs[i].i];
        if (vt == k_turn_sent || vt >= ut) {
            return true;
        }
    }
    return false;
}

static void que_claim (
    BktQ& b,
    u16* turn,
    u16* rem,
    u32 vi,
    u16 at,
    i32 nrem,
    u16* out_max) {
    turn[vi] = at;
    rem[vi] = rem_enc(nrem);
    if (at > *out_max) {
        *out_max = at;
    }
    bkt_push(b, vi, at, rem[vi]);
}

static i32 expand_pass (
    const Nbr* nbrs,
    u32 nn,
    i32 bud,
    u16 at,
    i32 k_mp,
    BktQ& b,
    u16* turn,
    u16* rem,
    u16* out_max) {
    i32 jump = 0;
    for (u32 i = 0; i < nn; ++i) {
        const Nbr& nb = nbrs[i];
        const i32 c = nb.c;
        if (bud >= c) {
            const i32 nrem = bud - c;
            if (better(turn[nb.i], rem[nb.i], at, nrem)) {
                que_claim(b, turn, rem, nb.i, at, nrem, out_max);
            }
            continue;
        }
        const i32 need = c - bud;
        const i32 n = (need + k_mp - 1) / k_mp;
        const i32 bud_n = bud + n * k_mp;
        const u16 at_n = static_cast<u16>(at + static_cast<u16>(n));
        if (!better(turn[nb.i], rem[nb.i], at_n, bud_n - c)) {
            continue;
        }
        if (jump == 0 || n < jump) {
            jump = n;
        }
    }
    return jump;
}

static void process_tile (
    const GameState& s,
    u16 w,
    u16 h,
    u16 ux,
    u16 uy,
    i32 k_mp,
    i32 k_min,
    BktQ& b,
    u16* turn,
    u16* rem,
    u16* fin_turn,
    u16* fin_rem,
    const Whiteboard_1B* mask,
    u16* out_max) {
    const u32 ui = tidx(w, ux, uy);
    const u16 ut = turn[ui];
    const u16 uraw = rem[ui];
    const i32 ur = rem_dec(uraw);
    if (ut >= k_turn_sent) {
        return;
    }
    if (fin_turn[ui] == ut && fin_rem[ui] == uraw) {
        return;
    }
    Nbr nbrs[8];
    const u32 nn = gather_nbrs(s, w, h, ux, uy, mask, nbrs);
    if (nn == 0u || !any_offer_possible(ut, nbrs, nn, turn)) {
        fin_turn[ui] = ut;
        fin_rem[ui] = uraw;
        return;
    }
    i32 bud = ur;
    u16 at = ut;
    if (bud < k_min) {
        at = static_cast<u16>(ut + 1u);
        bud = k_mp;
    }
    for (u16 guard = 0u; guard <= 64u; ++guard) {
        const i32 jump = expand_pass(nbrs, nn, bud, at, k_mp, b, turn, rem, out_max);
        if (jump <= 0) {
            break;
        }
        at = static_cast<u16>(at + static_cast<u16>(jump));
        bud += jump * k_mp;
    }
    fin_turn[ui] = turn[ui];
    fin_rem[ui] = rem[ui];
}

static bool flood (
    const GameState& s,
    u16 w,
    u16 h,
    u32 src_i,
    bool have_src,
    i32 k_mp,
    i32 k_min,
    u16* turn,
    u16* rem,
    BktQ& b,
    u16* fin_turn,
    u16* fin_rem,
    const Whiteboard_1B* mask,
    u16* out_max) {
    *out_max = 0;
    while (b.open_n > 0u) {
        while (b.min_t < k_bkt_n && b.heads[b.min_t] == k_nil) {
            b.min_t++;
        }
        if (have_src
            && turn[src_i] != k_turn_sent
            && fin_turn[src_i] == turn[src_i]
            && fin_rem[src_i] == rem[src_i]
            && b.min_t > turn[src_i]) {
            return true;
        }
        const u32 si = bkt_pop(b);
        if (si == k_nil) {
            break;
        }
        if (have_src
            && turn[src_i] != k_turn_sent
            && fin_turn[src_i] == turn[src_i]
            && fin_rem[src_i] == rem[src_i]
            && turn[si] > turn[src_i]) {
            continue;
        }
        process_tile(
            s, w, h, idx_x(si, w), idx_y(si, w), k_mp, k_min, b, turn, rem, fin_turn, fin_rem, mask,
            out_max);
    }
    if (!have_src) {
        return true;
    }
    return turn[src_i] != k_turn_sent;
}

static bool run_gen (
    const GameState& s,
    const RuntimeStatics& st,
    bool have_src,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    WalkP2P& walk,
    const Whiteboard_1B* mask,
    u16* out_max) {
    if (out_max == nullptr || !walk.ok() || !UnitMovementMng::mvt_ready()) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    if (w == 0 || h == 0 || w != walk.w() || h != walk.h()) {
        return false;
    }
    if (dst_x >= w || dst_y >= h) {
        return false;
    }
    if (have_src && (src_x >= w || src_y >= h)) {
        return false;
    }
    if (mask != nullptr && (!mask->ok() || mask->w() != w || mask->h() != h)) {
        return false;
    }
    if (!can_tile(s, dst_x, dst_y, mask)) {
        return false;
    }
    if (have_src && !can_tile(s, src_x, src_y, mask)) {
        return false;
    }
    const i32 k_mp = static_cast<i32>(st.config().get_mov_pt_per_turn());
    const i32 k_min = min_edge();
    if (k_mp <= 0 || k_min <= 0) {
        return false;
    }
    const u32 tile_n = s.m_map.tile_n();
    const u32 src_i = have_src ? tidx(w, src_x, src_y) : k_nil;
    const u32 dst_i = tidx(w, dst_x, dst_y);
    Whiteboard_1B inq_wb("GenerateDistanceP2P", "inq", 0u);
    Whiteboard_2B qt_wb("GenerateDistanceP2P", "qturn", 0u);
    Whiteboard_2B qr_wb("GenerateDistanceP2P", "qrem", 0u);
    Whiteboard_2B ft_wb("GenerateDistanceP2P", "fint", 0u);
    Whiteboard_2B fr_wb("GenerateDistanceP2P", "finr", 0u);
    Whiteboard_4B nxt_wb("GenerateDistanceP2P", "nxt", 0u);
    Whiteboard_4B prv_wb("GenerateDistanceP2P", "prv", 0u);
    u32 heads[k_bkt_n];
    if (!inq_wb.ok() || !qt_wb.ok() || !qr_wb.ok() || !ft_wb.ok() || !fr_wb.ok()
        || !nxt_wb.ok() || !prv_wb.ok()) {
        return false;
    }
    u16* turn = walk.turn();
    u16* rem = walk.rem();
    u8* in_q = inq_wb.get_iter_ptr();
    u16* q_turn = qt_wb.get_iter_ptr();
    u16* q_rem = qr_wb.get_iter_ptr();
    u16* fin_turn = ft_wb.get_iter_ptr();
    u16* fin_rem = fr_wb.get_iter_ptr();
    u32* nxt = nxt_wb.get_iter_ptr();
    u32* prv = prv_wb.get_iter_ptr();
    BktQ b;
    bkt_init(b, heads, nxt, prv, in_q, q_turn, q_rem, tile_n);
    for (u32 i = 0; i < tile_n; ++i) {
        turn[i] = k_turn_sent;
        rem[i] = rem_enc(0);
        fin_turn[i] = k_turn_sent;
        fin_rem[i] = 0u;
    }
    turn[dst_i] = 0u;
    rem[dst_i] = rem_enc(k_mp);
    bkt_push(b, dst_i, 0u, rem[dst_i]);
    *out_max = 0;
    if (have_src && src_i == dst_i) {
        return true;
    }
    const bool ok = flood(
        s, w, h, src_i, have_src, k_mp, k_min, turn, rem, b, fin_turn, fin_rem, mask, out_max);
    if (!ok) {
        for (u32 i = 0; i < tile_n; ++i) {
            turn[i] = k_turn_sent;
            rem[i] = rem_enc(0);
        }
        *out_max = 0;
    }
    return ok;
}

//================================================================================================================================
//=> - GenerateDistanceP2P -
//================================================================================================================================

bool GenerateDistanceP2P::generate (
    const GameState& s,
    const RuntimeStatics& st,
    u16 src_x,
    u16 src_y,
    u16 dst_x,
    u16 dst_y,
    WalkP2P& walk,
    const Whiteboard_1B* mask,
    u16* out_max) {
    return run_gen(s, st, true, src_x, src_y, dst_x, dst_y, walk, mask, out_max);
}

bool GenerateDistanceP2P::generate (
    const GameState& s,
    const RuntimeStatics& st,
    u16 dst_x,
    u16 dst_y,
    WalkP2P& walk,
    const Whiteboard_1B* mask,
    u16* out_max) {
    return run_gen(s, st, false, 0u, 0u, dst_x, dst_y, walk, mask, out_max);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
