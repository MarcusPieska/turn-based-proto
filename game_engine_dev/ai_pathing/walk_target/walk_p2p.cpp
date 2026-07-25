//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_p2p.h"

#include "generate_distance_p2p.h"
#include "game_array_simple.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "unit_movement_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_turn_sent = GenerateDistanceP2P::k_turn_sent;

//================================================================================================================================
//=> - WalkP2P -
//================================================================================================================================

WalkP2P::WalkP2P () :
    m_turn("WalkP2P", "turn", 0u),
    m_rem("WalkP2P", "rem", 0u) {
}

WalkP2P::~WalkP2P () {
}

bool WalkP2P::ok () const {
    return m_turn.ok() && m_rem.ok();
}

u16 WalkP2P::w () const {
    return m_turn.w();
}

u16 WalkP2P::h () const {
    return m_turn.h();
}

u16* WalkP2P::turn () {
    return m_turn.get_iter_ptr();
}

const u16* WalkP2P::turn () const {
    return m_turn.get_iter_ptr();
}

u16* WalkP2P::rem () {
    return m_rem.get_iter_ptr();
}

const u16* WalkP2P::rem () const {
    return m_rem.get_iter_ptr();
}

u32 WalkP2P::tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

i32 WalkP2P::rem_dec (u16 v) {
    return static_cast<i32>(static_cast<i16>(v));
}

bool WalkP2P::reach (const u16* turn, u16 w, u16 x, u16 y) {
    return turn[tidx(w, x, y)] != k_turn_sent;
}

bool WalkP2P::closer (u16 t, i32 r, u16 nt, i32 nr) {
    if (nt < t) {
        return true;
    }
    if (nt > t) {
        return false;
    }
    return nr > r;
}

bool WalkP2P::find_lo (
    u16 t,
    i32 r,
    const u16* turn,
    const u16* rem,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16& ox,
    u16& oy) {
    u16 bt = t;
    i32 br = r;
    u16 lx = x;
    u16 ly = y;
    bool have = false;
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 nx = static_cast<i32>(x) + MAP_NBR8_DX[k];
        const i32 ny = static_cast<i32>(y) + MAP_NBR8_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        if (!reach(turn, w, tx, ty)) {
            continue;
        }
        const u32 i = tidx(w, tx, ty);
        const u16 nt = turn[i];
        const i32 nr = rem_dec(rem[i]);
        if (!closer(t, r, nt, nr)) {
            continue;
        }
        if (!have || closer(bt, br, nt, nr)) {
            bt = nt;
            br = nr;
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

WalkP2P::StepRes WalkP2P::peek (const GameState& s, u16 x, u16 y) const {
    StepRes res = {};
    if (!ok() || !UnitMovementMng::mvt_ready()) {
        return res;
    }
    const u16 mw = s.m_map.width();
    const u16 mh = s.m_map.height();
    if (x >= mw || y >= mh || mw != w() || mh != h()) {
        return res;
    }
    const u16* tarr = turn();
    const u16* rarr = rem();
    const u32 i = tidx(mw, x, y);
    const u16 t = tarr[i];
    if (t >= k_turn_sent) {
        return res;
    }
    u16 tx = x;
    u16 ty = y;
    if (!find_lo(t, rem_dec(rarr[i]), tarr, rarr, mw, mh, x, y, tx, ty)) {
        return res;
    }
    const i16 c = UnitMovementMng::tile_cost(s, x, y, tx, ty);
    if (c <= 0) {
        return res;
    }
    res.nx = tx;
    res.ny = ty;
    res.cost = static_cast<u16>(c);
    res.have = true;
    return res;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
