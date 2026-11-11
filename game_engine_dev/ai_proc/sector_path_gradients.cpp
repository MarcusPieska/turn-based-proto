//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sector_path_gradients.h"

#include "generate_distance_p2p.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "unit_movement_mng.h"
#include "whiteboard_mng.h"

#include <cstring>

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

bool SectorPathGradients::s_ok = false;
u16 SectorPathGradients::s_cw = 0u;
u16 SectorPathGradients::s_ch = 0u;
u16 SectorPathGradients::s_dw = 0u;
u16 SectorPathGradients::s_dh = 0u;
u16 SectorPathGradients::s_map_w = 0u;
u16 SectorPathGradients::s_map_h = 0u;
u32 SectorPathGradients::s_tile_n = 0u;
u16 SectorPathGradients::s_sec_n = 0u;
u16 SectorPathGradients::s_fit_one = 0u;
u16 SectorPathGradients::s_fit_dual = 0u;
u16* SectorPathGradients::s_min_x = nullptr;
u16* SectorPathGradients::s_min_y = nullptr;
u16* SectorPathGradients::s_max_x = nullptr;
u16* SectorPathGradients::s_max_y = nullptr;
SectorPathGradients SectorPathGradients::s_pool[SectorPathGradients::k_pool_n];
bool SectorPathGradients::s_pool_on = false;

//================================================================================================================================
//=> - SectorPathGradients -
//================================================================================================================================

SectorPathGradients::SectorPathGradients () :
    m_n_one(0u),
    m_n_dual(0u),
    m_own_one(nullptr),
    m_own_one_n(0u),
    m_own_dual(nullptr),
    m_own_dual_n(0u),
    m_sec(nullptr) {
    for (u16 i = 0; i < k_wh_cap; ++i) {
        m_tn_one[i] = nullptr;
        m_rm_one[i] = nullptr;
        m_use_one[i] = 0u;
        m_tn_dual[i] = nullptr;
        m_rm_dual[i] = nullptr;
        m_use_dual[i] = 0u;
    }
    for (u16 i = 0; i < k_cap; ++i) {
        m_full[i] = nullptr;
        m_mode[i] = k_off;
        m_wh_i[i] = U16_KEY_NULL;
        m_chk_i[i] = U16_KEY_NULL;
        m_ox[i] = 0u;
        m_oy[i] = 0u;
        m_cw[i] = 0u;
        m_ch[i] = 0u;
        m_sa[i] = U16_KEY_NULL;
        m_sb[i] = U16_KEY_NULL;
        m_pn[i] = 0u;
        m_sx[i] = 0u;
        m_sy[i] = 0u;
        m_pi[i] = 0u;
        m_ax[i] = 0u;
        m_ay[i] = 0u;
        m_on[i] = 0u;
    }
}

SectorPathGradients::~SectorPathGradients () {
    clr();
}

bool SectorPathGradients::setup_ok () {
    return s_ok;
}

bool SectorPathGradients::pool_setup (const Whiteboard_2B& sec, u16 sec_n) {
    pool_teardown();
    if (!setup(sec, sec_n)) {
        return false;
    }
    for (u16 i = 0; i < k_pool_n; ++i) {
        if (!s_pool[i].bind(&sec)) {
            pool_teardown();
            return false;
        }
    }
    s_pool_on = true;
    return true;
}

void SectorPathGradients::pool_teardown () {
    for (u16 i = 0; i < k_pool_n; ++i) {
        s_pool[i].clr();
    }
    s_pool_on = false;
    teardown();
}

SectorPathGradients* SectorPathGradients::pool_bank (u16 i) {
    if (!s_pool_on || i >= k_pool_n) {
        return nullptr;
    }
    return &s_pool[i];
}

SectorPathGradients* SectorPathGradients::pool_acq (u16* out_slot) {
    if (!s_pool_on || out_slot == nullptr) {
        return nullptr;
    }
    for (u16 i = 0; i < k_pool_n; ++i) {
        const u16 slot = s_pool[i].free_slot();
        if (slot != U16_KEY_NULL) {
            *out_slot = slot;
            return &s_pool[i];
        }
    }
    return nullptr;
}

bool SectorPathGradients::slot_ok (u16 i) const {
    return i < k_cap && m_on[i] != 0u && m_mode[i] != k_off;
}

bool SectorPathGradients::ok (u16 i) const {
    return slot_ok(i);
}

u16 SectorPathGradients::free_slot () const {
    for (u16 i = 0; i < k_cap; ++i) {
        if (m_on[i] == 0u) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

u16 SectorPathGradients::chunk_n () const {
    u16 n = 0u;
    for (u16 i = 0; i < k_cap; ++i) {
        if (m_on[i] != 0u && (m_mode[i] == k_one || m_mode[i] == k_dual)) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

u16 SectorPathGradients::wb_n () const {
    u16 n = 0u;
    for (u16 i = 0; i < k_cap; ++i) {
        if (m_full[i] != nullptr && m_full[i]->ok()) {
            n = static_cast<u16>(n + 2u);
        }
    }
    return n;
}

bool SectorPathGradients::ens_bank (u16** tn, u16** rm, u16* n, u16 bi, u32 cells) {
    if (bi >= k_wh_cap || cells == 0u) {
        return false;
    }
    if (tn[bi] == nullptr) {
        tn[bi] = new u16[cells];
        rm[bi] = new u16[cells];
        if (tn[bi] == nullptr || rm[bi] == nullptr) {
            delete[] tn[bi];
            delete[] rm[bi];
            tn[bi] = nullptr;
            rm[bi] = nullptr;
            return false;
        }
        if (bi >= *n) {
            *n = static_cast<u16>(bi + 1u);
        }
    }
    return tn[bi] != nullptr && rm[bi] != nullptr;
}

void SectorPathGradients::drop_bank (u16** tn, u16** rm, u16* n, u16* use, u16 bi) {
    if (bi >= k_wh_cap || tn[bi] == nullptr) {
        return;
    }
    if (use[bi] != 0u) {
        return;
    }
    delete[] tn[bi];
    delete[] rm[bi];
    tn[bi] = nullptr;
    rm[bi] = nullptr;
    if (*n > 0u && bi == static_cast<u16>(*n - 1u)) {
        while (*n > 0u && tn[*n - 1u] == nullptr) {
            *n = static_cast<u16>(*n - 1u);
        }
    }
}

u16* SectorPathGradients::turn_ptr (u16 i) const {
    if (!slot_ok(i)) {
        return nullptr;
    }
    if (m_mode[i] == k_full) {
        return (m_full[i] != nullptr) ? m_full[i]->turn() : nullptr;
    }
    const bool dual = (m_mode[i] == k_dual);
    u16* tn = dual ? m_tn_dual[m_wh_i[i]] : m_tn_one[m_wh_i[i]];
    if (tn == nullptr) {
        return nullptr;
    }
    const u32 cells = static_cast<u32>(m_cw[i]) * static_cast<u32>(m_ch[i]);
    const u16 local = dual
        ? static_cast<u16>(m_chk_i[i] % s_fit_dual)
        : static_cast<u16>(m_chk_i[i] % s_fit_one);
    return tn + static_cast<u32>(local) * cells;
}

u16* SectorPathGradients::rem_ptr (u16 i) const {
    if (!slot_ok(i)) {
        return nullptr;
    }
    if (m_mode[i] == k_full) {
        return (m_full[i] != nullptr) ? m_full[i]->rem() : nullptr;
    }
    if (m_mode[i] == k_path) {
        return nullptr;
    }
    const bool dual = (m_mode[i] == k_dual);
    u16* rm = dual ? m_rm_dual[m_wh_i[i]] : m_rm_one[m_wh_i[i]];
    if (rm == nullptr) {
        return nullptr;
    }
    const u32 cells = static_cast<u32>(m_cw[i]) * static_cast<u32>(m_ch[i]);
    const u16 local = dual
        ? static_cast<u16>(m_chk_i[i] % s_fit_dual)
        : static_cast<u16>(m_chk_i[i] % s_fit_one);
    return rm + static_cast<u32>(local) * cells;
}

bool SectorPathGradients::covers (u16 slot, u16 x, u16 y) const {
    if (!slot_ok(slot)) {
        return false;
    }
    if (m_mode[slot] == k_full || m_mode[slot] == k_path) {
        return true;
    }
    return x >= m_ox[slot] && y >= m_oy[slot]
        && x < static_cast<u16>(m_ox[slot] + m_cw[slot])
        && y < static_cast<u16>(m_oy[slot] + m_ch[slot]);
}

bool SectorPathGradients::clip_mask (u16 slot, Whiteboard_1B& mask) const {
    (void)slot;
    (void)mask;
    return true;
}

static bool spg_closer (u16 t, i32 r, u16 nt, i32 nr) {
    if (nt < t) {
        return true;
    }
    if (nt > t) {
        return false;
    }
    return nr > r;
}

SectorPathGradients::StepRes SectorPathGradients::peek_path (u16 i, const GameState& s, u16 x, u16 y) const {
    StepRes r;
    r.nx = 0u;
    r.ny = 0u;
    r.cost = 0u;
    r.have = false;
    r.reason = PeekFail::NotReady;
    u16* tape = turn_ptr(i);
    if (tape == nullptr) {
        return r;
    }
    if (m_ax[i] != x || m_ay[i] != y) {
        bool step_ok = false;
        if (m_pi[i] < m_pn[i]) {
            const u16 d0 = tape[m_pi[i]];
            if (d0 < MAP_NBR8_N) {
                const i32 nx0 = static_cast<i32>(m_ax[i]) + MAP_NBR8_DX[d0];
                const i32 ny0 = static_cast<i32>(m_ay[i]) + MAP_NBR8_DY[d0];
                if (nx0 >= 0 && ny0 >= 0
                    && static_cast<u16>(nx0) == x && static_cast<u16>(ny0) == y) {
                    m_ax[i] = x;
                    m_ay[i] = y;
                    m_pi[i] = static_cast<u16>(m_pi[i] + 1u);
                    step_ok = true;
                }
            }
        }
        if (!step_ok) {
            u16 cx = m_sx[i];
            u16 cy = m_sy[i];
            u16 k = 0u;
            bool hit = false;
            while (k <= m_pn[i]) {
                if (cx == x && cy == y) {
                    m_ax[i] = x;
                    m_ay[i] = y;
                    m_pi[i] = k;
                    hit = true;
                    break;
                }
                if (k >= m_pn[i]) {
                    break;
                }
                const u16 d = tape[k];
                if (d >= MAP_NBR8_N) {
                    r.reason = PeekFail::Unreach;
                    return r;
                }
                const i32 nx = static_cast<i32>(cx) + MAP_NBR8_DX[d];
                const i32 ny = static_cast<i32>(cy) + MAP_NBR8_DY[d];
                if (nx < 0 || ny < 0) {
                    r.reason = PeekFail::BadPos;
                    return r;
                }
                cx = static_cast<u16>(nx);
                cy = static_cast<u16>(ny);
                k = static_cast<u16>(k + 1u);
            }
            if (!hit) {
                r.reason = PeekFail::Unreach;
                return r;
            }
        }
    }
    if (m_pi[i] >= m_pn[i]) {
        r.reason = PeekFail::NoDown;
        return r;
    }
    const u16 d = tape[m_pi[i]];
    if (d >= MAP_NBR8_N) {
        r.reason = PeekFail::Unreach;
        return r;
    }
    const i32 nx = static_cast<i32>(m_ax[i]) + MAP_NBR8_DX[d];
    const i32 ny = static_cast<i32>(m_ay[i]) + MAP_NBR8_DY[d];
    if (nx < 0 || ny < 0) {
        r.reason = PeekFail::BadPos;
        return r;
    }
    const u16 bx = static_cast<u16>(nx);
    const u16 by = static_cast<u16>(ny);
    const i16 c = UnitMovementMng::tile_cost(s, x, y, bx, by);
    if (c <= 0) {
        r.reason = PeekFail::Cost;
        return r;
    }
    r.nx = bx;
    r.ny = by;
    r.cost = static_cast<u16>(c);
    r.have = true;
    r.reason = PeekFail::Ok;
    return r;
}

SectorPathGradients::StepRes SectorPathGradients::peek (u16 i, const GameState& s, u16 x, u16 y) const {
    StepRes r;
    r.nx = 0u;
    r.ny = 0u;
    r.cost = 0u;
    r.have = false;
    r.reason = PeekFail::NotReady;
    if (!slot_ok(i) || !UnitMovementMng::mvt_ready()) {
        return r;
    }
    if (m_mode[i] == k_path) {
        return peek_path(i, s, x, y);
    }
    if (m_mode[i] == k_full) {
        if (m_full[i] == nullptr) {
            return r;
        }
        return m_full[i]->peek(s, x, y);
    }
    if (!covers(i, x, y)) {
        r.reason = PeekFail::Unreach;
        return r;
    }
    u16* turn = turn_ptr(i);
    u16* rem = rem_ptr(i);
    if (turn == nullptr || rem == nullptr) {
        return r;
    }
    const u16 cw = m_cw[i];
    const u16 ch = m_ch[i];
    const u16 ox = m_ox[i];
    const u16 oy = m_oy[i];
    const u16 lx = static_cast<u16>(x - ox);
    const u16 ly = static_cast<u16>(y - oy);
    const u32 li = static_cast<u32>(ly) * static_cast<u32>(cw) + static_cast<u32>(lx);
    const u16 t = turn[li];
    if (t == GenerateDistanceP2P::k_turn_sent) {
        r.reason = PeekFail::Unreach;
        return r;
    }
    const i32 rr = static_cast<i32>(static_cast<i16>(rem[li]));
    bool have = false;
    u16 bx = x;
    u16 by = y;
    u16 bt = t;
    i32 br = rr;
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 nx = static_cast<i32>(x) + MAP_NBR8_DX[k];
        const i32 ny = static_cast<i32>(y) + MAP_NBR8_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (!covers(i, tx, ty)) {
            continue;
        }
        const u16 tlx = static_cast<u16>(tx - ox);
        const u16 tly = static_cast<u16>(ty - oy);
        if (tlx >= cw || tly >= ch) {
            continue;
        }
        const u32 ti = static_cast<u32>(tly) * static_cast<u32>(cw) + static_cast<u32>(tlx);
        const u16 nt = turn[ti];
        if (nt == GenerateDistanceP2P::k_turn_sent) {
            continue;
        }
        const i32 nr = static_cast<i32>(static_cast<i16>(rem[ti]));
        if (!spg_closer(t, rr, nt, nr)) {
            continue;
        }
        if (!have || spg_closer(bt, br, nt, nr)) {
            have = true;
            bx = tx;
            by = ty;
            bt = nt;
            br = nr;
        }
    }
    if (!have) {
        r.reason = PeekFail::NoDown;
        return r;
    }
    const i16 c = UnitMovementMng::tile_cost(s, x, y, bx, by);
    if (c <= 0) {
        r.reason = PeekFail::Cost;
        return r;
    }
    r.nx = bx;
    r.ny = by;
    r.cost = static_cast<u16>(c);
    r.have = true;
    r.reason = PeekFail::Ok;
    return r;
}

void SectorPathGradients::peek_opts (u16 i, const GameState& s, u16 x, u16 y, u16* ox, u16* oy) const {
    if (ox == nullptr || oy == nullptr) {
        return;
    }
    for (u16 k = 0; k < k_opt_n; ++k) {
        ox[k] = U16_KEY_NULL;
        oy[k] = U16_KEY_NULL;
    }
    if (!slot_ok(i)) {
        return;
    }
    if (m_mode[i] == k_full) {
        if (m_full[i] != nullptr) {
            m_full[i]->peek_opts(s, x, y, ox, oy);
        }
        return;
    }
    StepRes step = peek(i, s, x, y);
    if (step.have) {
        ox[0] = step.nx;
        oy[0] = step.ny;
    }
}

//================================================================================================================================
//=> - Impl select -
//================================================================================================================================

#ifndef SECTOR_PATH_GRADIENTS_IMPL
#define SECTOR_PATH_GRADIENTS_IMPL "impl/sector_path_gradients_impl_mk03.cpp"
#endif

#include SECTOR_PATH_GRADIENTS_IMPL

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
