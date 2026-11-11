//================================================================================================================================
//=> - SectorPathGradients mk03 -
//================================================================================================================================
//
//  Chunk atlas in heap turn/rem banks. setup() sizes chunks from max sector AABB (dual = 2x). lease
//  places a sector (or dual AABB) into a free chunk; fill Dial's into that window (sector-clipped).
//  pack_path: temp full WalkP2P Dial, then store dir tape in a one-chunk (k_path). Failures LOG_SPG_FAIL.
//
//================================================================================================================================

#include "generate_distance_p2p.h"
#include "game_map_grid_defs.h"
#include "game_state.h"
#include "log_dbg/log_spg_fail.h"
#include "runtime_statics.h"
#include "walk_p2p.h"

#include <cstring>

enum : u16 {
    k_fail_bad_sec = 0u,
    k_fail_empty = 1u,
    k_fail_dual_big = 2u,
    k_fail_no_chunk = 3u,
    k_fail_outside = 4u,
    k_fail_dial = 5u,
    k_fail_args = 6u,
    k_fail_ready = 7u
};

static void spg_fail (u32 turn, u16 slot, u16 reason, u16 a, u16 b) {
    LOG_SPG_FAIL::LOG(turn, SC_U32(slot), SC_U32(reason), SC_U32(a), SC_U32(b));
}

bool SectorPathGradients::setup (const Whiteboard_2B& sec, u16 sec_n) {
    teardown();
    if (!sec.ok() || sec_n == 0u) {
        return false;
    }
    s_map_w = sec.w();
    s_map_h = sec.h();
    s_tile_n = static_cast<u32>(s_map_w) * static_cast<u32>(s_map_h);
    s_sec_n = sec_n;
    s_min_x = new u16[sec_n];
    s_min_y = new u16[sec_n];
    s_max_x = new u16[sec_n];
    s_max_y = new u16[sec_n];
    if (s_min_x == nullptr || s_min_y == nullptr || s_max_x == nullptr || s_max_y == nullptr) {
        teardown();
        return false;
    }
    for (u16 i = 0; i < sec_n; ++i) {
        s_min_x[i] = s_map_w;
        s_min_y[i] = s_map_h;
        s_max_x[i] = 0u;
        s_max_y[i] = 0u;
    }
    u16 have = 0u;
    for (u16 y = 0; y < s_map_h; ++y) {
        for (u16 x = 0; x < s_map_w; ++x) {
            const u16 tag = sec.rd(x, y);
            if (tag == 0u) {
                continue;
            }
            const u16 si = static_cast<u16>(tag - 1u);
            if (si >= sec_n) {
                continue;
            }
            if (s_min_x[si] == s_map_w) {
                have = static_cast<u16>(have + 1u);
            }
            if (x < s_min_x[si]) {
                s_min_x[si] = x;
            }
            if (y < s_min_y[si]) {
                s_min_y[si] = y;
            }
            if (x > s_max_x[si]) {
                s_max_x[si] = x;
            }
            if (y > s_max_y[si]) {
                s_max_y[si] = y;
            }
        }
    }
    if (have == 0u) {
        teardown();
        return false;
    }
    u16 cw = 1u;
    u16 ch = 1u;
    for (u16 i = 0; i < sec_n; ++i) {
        if (s_min_x[i] > s_max_x[i] || s_min_y[i] > s_max_y[i]) {
            continue;
        }
        const u16 w = static_cast<u16>(s_max_x[i] - s_min_x[i] + 1u);
        const u16 h = static_cast<u16>(s_max_y[i] - s_min_y[i] + 1u);
        if (w > cw) {
            cw = w;
        }
        if (h > ch) {
            ch = h;
        }
    }
    s_cw = cw;
    s_ch = ch;
    s_dw = static_cast<u16>(cw * 2u);
    s_dh = static_cast<u16>(ch * 2u);
    if (s_dw > s_map_w) {
        s_dw = s_map_w;
    }
    if (s_dh > s_map_h) {
        s_dh = s_map_h;
    }
    const u32 one_cells = static_cast<u32>(s_cw) * static_cast<u32>(s_ch);
    const u32 dual_cells = static_cast<u32>(s_dw) * static_cast<u32>(s_dh);
    if (one_cells == 0u || dual_cells == 0u || one_cells > s_tile_n || dual_cells > s_tile_n) {
        teardown();
        return false;
    }
    s_fit_one = static_cast<u16>(s_tile_n / one_cells);
    s_fit_dual = static_cast<u16>(s_tile_n / dual_cells);
    if (s_fit_one == 0u || s_fit_dual == 0u) {
        teardown();
        return false;
    }
    s_ok = true;
    return true;
}

void SectorPathGradients::teardown () {
    delete[] s_min_x;
    delete[] s_min_y;
    delete[] s_max_x;
    delete[] s_max_y;
    s_min_x = nullptr;
    s_min_y = nullptr;
    s_max_x = nullptr;
    s_max_y = nullptr;
    s_cw = 0u;
    s_ch = 0u;
    s_dw = 0u;
    s_dh = 0u;
    s_map_w = 0u;
    s_map_h = 0u;
    s_tile_n = 0u;
    s_sec_n = 0u;
    s_fit_one = 0u;
    s_fit_dual = 0u;
    s_ok = false;
}

bool SectorPathGradients::bind (const Whiteboard_2B* sec) {
    m_sec = sec;
    if (!s_ok || s_fit_one == 0u) {
        return false;
    }
    if (m_own_one == nullptr) {
        m_own_one_n = static_cast<u16>(s_fit_one * k_bank_mul);
        m_own_one = new u16[m_own_one_n];
        if (m_own_one == nullptr) {
            return false;
        }
        for (u16 i = 0; i < m_own_one_n; ++i) {
            m_own_one[i] = U16_KEY_NULL;
        }
    }
    if (m_own_dual == nullptr) {
        m_own_dual_n = static_cast<u16>(s_fit_dual * k_bank_mul);
        m_own_dual = new u16[m_own_dual_n];
        if (m_own_dual == nullptr) {
            return false;
        }
        for (u16 i = 0; i < m_own_dual_n; ++i) {
            m_own_dual[i] = U16_KEY_NULL;
        }
    }
    return true;
}

void SectorPathGradients::clr () {
    for (u16 i = 0; i < k_cap; ++i) {
        release(i);
        delete m_full[i];
        m_full[i] = nullptr;
    }
    for (u16 i = 0; i < k_wh_cap; ++i) {
        delete[] m_tn_one[i];
        delete[] m_rm_one[i];
        m_tn_one[i] = nullptr;
        m_rm_one[i] = nullptr;
        m_use_one[i] = 0u;
        delete[] m_tn_dual[i];
        delete[] m_rm_dual[i];
        m_tn_dual[i] = nullptr;
        m_rm_dual[i] = nullptr;
        m_use_dual[i] = 0u;
    }
    m_n_one = 0u;
    m_n_dual = 0u;
    delete[] m_own_one;
    delete[] m_own_dual;
    m_own_one = nullptr;
    m_own_dual = nullptr;
    m_own_one_n = 0u;
    m_own_dual_n = 0u;
    m_sec = nullptr;
}

void SectorPathGradients::release (u16 slot) {
    if (slot >= k_cap || m_on[slot] == 0u) {
        return;
    }
    if ((m_mode[slot] == k_one || m_mode[slot] == k_path) && m_chk_i[slot] < m_own_one_n) {
        m_own_one[m_chk_i[slot]] = U16_KEY_NULL;
        if (m_wh_i[slot] < k_wh_cap && m_use_one[m_wh_i[slot]] > 0u) {
            m_use_one[m_wh_i[slot]] = static_cast<u16>(m_use_one[m_wh_i[slot]] - 1u);
            drop_bank(m_tn_one, m_rm_one, &m_n_one, m_use_one, m_wh_i[slot]);
        }
    } else if (m_mode[slot] == k_dual && m_chk_i[slot] < m_own_dual_n) {
        m_own_dual[m_chk_i[slot]] = U16_KEY_NULL;
        if (m_wh_i[slot] < k_wh_cap && m_use_dual[m_wh_i[slot]] > 0u) {
            m_use_dual[m_wh_i[slot]] = static_cast<u16>(m_use_dual[m_wh_i[slot]] - 1u);
            drop_bank(m_tn_dual, m_rm_dual, &m_n_dual, m_use_dual, m_wh_i[slot]);
        }
    }
    m_on[slot] = 0u;
    m_mode[slot] = k_off;
    m_wh_i[slot] = U16_KEY_NULL;
    m_chk_i[slot] = U16_KEY_NULL;
    m_sa[slot] = U16_KEY_NULL;
    m_sb[slot] = U16_KEY_NULL;
    m_pn[slot] = 0u;
    m_sx[slot] = 0u;
    m_sy[slot] = 0u;
    m_pi[slot] = 0u;
    m_ax[slot] = 0u;
    m_ay[slot] = 0u;
}

bool SectorPathGradients::take_full (u16 slot) {
    spg_fail(0u, slot, k_fail_args, 0u, 0u);
    return false;
}

bool SectorPathGradients::take_chunk (u8 kind, u16 slot, u16 ox, u16 oy, u16 cw, u16 ch) {
    if (slot >= k_cap || !s_ok) {
        spg_fail(0u, slot, k_fail_ready, kind, 0u);
        return false;
    }
    release(slot);
    u16* own = (kind == k_dual) ? m_own_dual : m_own_one;
    u16 own_n = (kind == k_dual) ? m_own_dual_n : m_own_one_n;
    u16 fit = (kind == k_dual) ? s_fit_dual : s_fit_one;
    u16** tn = (kind == k_dual) ? m_tn_dual : m_tn_one;
    u16** rm = (kind == k_dual) ? m_rm_dual : m_rm_one;
    u16* use = (kind == k_dual) ? m_use_dual : m_use_one;
    u16* n = (kind == k_dual) ? &m_n_dual : &m_n_one;
    const u32 cells = (kind == k_dual)
        ? static_cast<u32>(s_fit_dual) * static_cast<u32>(s_dw) * static_cast<u32>(s_dh)
        : static_cast<u32>(s_fit_one) * static_cast<u32>(s_cw) * static_cast<u32>(s_ch);
    if (own == nullptr || fit == 0u || cells == 0u) {
        spg_fail(0u, slot, k_fail_no_chunk, kind, 0u);
        return false;
    }
    u16 gi = U16_KEY_NULL;
    for (u16 i = 0; i < own_n; ++i) {
        if (own[i] == U16_KEY_NULL) {
            gi = i;
            break;
        }
    }
    if (gi == U16_KEY_NULL) {
        spg_fail(0u, slot, k_fail_no_chunk, kind, own_n);
        return false;
    }
    const u16 bi = static_cast<u16>(gi / fit);
    if (!ens_bank(tn, rm, n, bi, cells)) {
        spg_fail(0u, slot, k_fail_no_chunk, kind, bi);
        return false;
    }
    own[gi] = slot;
    use[bi] = static_cast<u16>(use[bi] + 1u);
    m_mode[slot] = kind;
    m_wh_i[slot] = bi;
    m_chk_i[slot] = gi;
    m_ox[slot] = ox;
    m_oy[slot] = oy;
    m_cw[slot] = cw;
    m_ch[slot] = ch;
    m_on[slot] = 1u;
    return true;
}

bool SectorPathGradients::lease (u16 slot, u16 sector) {
    if (sector == U16_KEY_NULL || !s_ok || sector >= s_sec_n) {
        spg_fail(0u, slot, k_fail_bad_sec, sector, 0u);
        return false;
    }
    if (s_min_x[sector] > s_max_x[sector]) {
        spg_fail(0u, slot, k_fail_empty, sector, 0u);
        return false;
    }
    if (!take_chunk(k_one, slot, s_min_x[sector], s_min_y[sector], s_cw, s_ch)) {
        return false;
    }
    m_sa[slot] = sector;
    m_sb[slot] = U16_KEY_NULL;
    return true;
}

bool SectorPathGradients::lease_multi (u16 slot, const u16* sectors, u16 n) {
    if (sectors == nullptr || n == 0u) {
        spg_fail(0u, slot, k_fail_args, n, 0u);
        return false;
    }
    if (n == 1u) {
        return lease(slot, sectors[0]);
    }
    if (!s_ok) {
        spg_fail(0u, slot, k_fail_ready, n, 0u);
        return false;
    }
    u16 a = U16_KEY_NULL;
    u16 b = U16_KEY_NULL;
    for (u16 i = 0; i < n; ++i) {
        if (sectors[i] == U16_KEY_NULL || sectors[i] >= s_sec_n) {
            continue;
        }
        if (a == U16_KEY_NULL) {
            a = sectors[i];
        } else if (sectors[i] != a) {
            b = sectors[i];
            break;
        }
    }
    if (a == U16_KEY_NULL) {
        spg_fail(0u, slot, k_fail_bad_sec, 0u, 0u);
        return false;
    }
    if (b == U16_KEY_NULL) {
        return lease(slot, a);
    }
    if (s_min_x[a] > s_max_x[a] || s_min_x[b] > s_max_x[b]) {
        spg_fail(0u, slot, k_fail_empty, a, b);
        return false;
    }
    const u16 minx = (s_min_x[a] < s_min_x[b]) ? s_min_x[a] : s_min_x[b];
    const u16 miny = (s_min_y[a] < s_min_y[b]) ? s_min_y[a] : s_min_y[b];
    const u16 maxx = (s_max_x[a] > s_max_x[b]) ? s_max_x[a] : s_max_x[b];
    const u16 maxy = (s_max_y[a] > s_max_y[b]) ? s_max_y[a] : s_max_y[b];
    const u16 bw = static_cast<u16>(maxx - minx + 1u);
    const u16 bh = static_cast<u16>(maxy - miny + 1u);
    if (bw > s_dw || bh > s_dh) {
        spg_fail(0u, slot, k_fail_dual_big, bw, bh);
        return false;
    }
    if (!take_chunk(k_dual, slot, minx, miny, s_dw, s_dh)) {
        return false;
    }
    m_sa[slot] = a;
    m_sb[slot] = b;
    return true;
}

bool SectorPathGradients::fill (
    u16 slot,
    GameState& st,
    const RuntimeStatics& rst,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    const Whiteboard_1B* mask) {
    const u32 turn = st.m_current_turn;
    if (!slot_ok(slot) || st.m_statics == nullptr) {
        spg_fail(turn, slot, k_fail_ready, sx, sy);
        return false;
    }
    if (m_mode[slot] == k_full || m_mode[slot] == k_path) {
        spg_fail(turn, slot, k_fail_args, 0u, 0u);
        return false;
    }
    if (!covers(slot, sx, sy) || !covers(slot, dx, dy)) {
        spg_fail(turn, slot, k_fail_outside, sx, dx);
        return false;
    }
    u16* turn_b = turn_ptr(slot);
    u16* rem_b = rem_ptr(slot);
    if (turn_b == nullptr || rem_b == nullptr) {
        spg_fail(turn, slot, k_fail_no_chunk, 0u, 0u);
        return false;
    }
    u16 dmax = 0u;
    if (!GenerateDistanceP2P::generate(
            st,
            rst,
            sx,
            sy,
            dx,
            dy,
            turn_b,
            rem_b,
            m_ox[slot],
            m_oy[slot],
            m_cw[slot],
            m_ch[slot],
            mask,
            m_sec,
            m_sa[slot],
            m_sb[slot],
            &dmax)) {
        spg_fail(turn, slot, k_fail_dial, m_sa[slot], m_sb[slot]);
        return false;
    }
    return true;
}

bool SectorPathGradients::pack_path (
    u16 slot,
    GameState& st,
    const RuntimeStatics& rst,
    u16 sx,
    u16 sy,
    u16 dx,
    u16 dy,
    const Whiteboard_1B* mask) {
    const u32 turn = st.m_current_turn;
    if (slot >= k_cap || !s_ok || st.m_statics == nullptr) {
        spg_fail(turn, slot, k_fail_ready, sx, sy);
        return false;
    }
    WalkP2P walk;
    u16 dmax = 0u;
    if (!GenerateDistanceP2P::generate(st, rst, sx, sy, dx, dy, walk, mask, &dmax)) {
        spg_fail(turn, slot, k_fail_dial, sx, dx);
        return false;
    }
    if (!take_chunk(k_one, slot, 0u, 0u, s_cw, s_ch)) {
        return false;
    }
    m_mode[slot] = k_path;
    m_sa[slot] = U16_KEY_NULL;
    m_sb[slot] = U16_KEY_NULL;
    u16* tape = turn_ptr(slot);
    if (tape == nullptr) {
        release(slot);
        spg_fail(turn, slot, k_fail_no_chunk, 0u, 0u);
        return false;
    }
    const u32 cap = static_cast<u32>(s_cw) * static_cast<u32>(s_ch);
    u16 x = sx;
    u16 y = sy;
    u16 n = 0u;
    while (x != dx || y != dy) {
        if (static_cast<u32>(n) >= cap) {
            release(slot);
            spg_fail(turn, slot, k_fail_args, n, 0u);
            return false;
        }
        const WalkP2P::StepRes step = walk.peek(st, x, y);
        if (!step.have) {
            release(slot);
            spg_fail(turn, slot, k_fail_dial, x, y);
            return false;
        }
        u16 d = MAP_NBR8_N;
        for (u16 k = 0u; k < MAP_NBR8_N; ++k) {
            if (static_cast<i32>(x) + MAP_NBR8_DX[k] == static_cast<i32>(step.nx)
                && static_cast<i32>(y) + MAP_NBR8_DY[k] == static_cast<i32>(step.ny)) {
                d = k;
                break;
            }
        }
        if (d >= MAP_NBR8_N) {
            release(slot);
            spg_fail(turn, slot, k_fail_args, step.nx, step.ny);
            return false;
        }
        tape[n++] = d;
        x = step.nx;
        y = step.ny;
    }
    m_pn[slot] = n;
    m_sx[slot] = sx;
    m_sy[slot] = sy;
    m_pi[slot] = 0u;
    m_ax[slot] = sx;
    m_ay[slot] = sy;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
