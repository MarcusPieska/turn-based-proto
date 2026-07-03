//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "river_explore_mk2.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const i32 k_dx4[4] = {0, 0, -1, 1};
static const i32 k_dy4[4] = {-1, 1, 0, 0};

//================================================================================================================================
//=> - RiverExploreMk2 -
//================================================================================================================================

RiverExploreMk2::RiverExploreMk2 ( const GameArraySimple& map, MapBitOverlay& explored,  u16 sx, u16 sy, u16 sight, u8 player) :
    m_map(map),
    m_exp(explored),
    m_x(sx),
    m_y(sy),
    m_sx(sx),
    m_sy(sy),
    m_sight(sight),
    m_player(player),
    m_ph(k_ph_done),
    m_greedy_ph(false),
    m_has_blk(false),
    m_blk_x(0),
    m_blk_y(0),
    m_hn(0),
    m_hp(0),
    m_bck_n(0),
    m_bck_t(0),
    m_gs_n(0),
    m_ge_n(0) {
    if (is_riv(m_x, m_y)) {
        start_greedy_ph();
    }
}

void RiverExploreMk2::gs_push (u16 x, u16 y) {
    if (m_gs_n < k_mk) {
        m_gsx[m_gs_n] = x;
        m_gsy[m_gs_n] = y;
        ++m_gs_n;
    }
}

void RiverExploreMk2::ge_push (u16 x, u16 y) {
    if (m_ge_n < k_mk) {
        m_gex[m_ge_n] = x;
        m_gey[m_ge_n] = y;
        ++m_ge_n;
    }
}

void RiverExploreMk2::hist_push (u16 x, u16 y) {
    m_hx[m_hp] = x;
    m_hy[m_hp] = y;
    m_hp = (m_hp + 1u) % k_hist;
    if (m_hn < k_hist) {
        ++m_hn;
    }
}

bool RiverExploreMk2::hist_pop (u16& x, u16& y) {
    if (m_hn == 0u) {
        return false;
    }
    m_hp = (m_hp + k_hist - 1u) % k_hist;
    --m_hn;
    x = m_hx[m_hp];
    y = m_hy[m_hp];
    return true;
}

void RiverExploreMk2::bck_clr () {
    m_bck_n = 0;
    m_bck_t = 0;
}

void RiverExploreMk2::bck_push (u16 x, u16 y) {
    if (bck_has(x, y)) {
        return;
    }
    m_bck_x[m_bck_t] = x;
    m_bck_y[m_bck_t] = y;
    m_bck_t = (m_bck_t + 1u) % k_q;
    if (m_bck_n < k_q) {
        ++m_bck_n;
    }
}

bool RiverExploreMk2::bck_has (u16 x, u16 y) const {
    for (u16 i = 0; i < m_bck_n; ++i) {
        const u16 j = static_cast<u16>((m_bck_t + k_q - m_bck_n + i) % k_q);
        if (m_bck_x[j] == x && m_bck_y[j] == y) {
            return true;
        }
    }
    return false;
}

bool RiverExploreMk2::hist8_has (u16 x, u16 y) const {
    const u16 n = (m_hn < k_q) ? m_hn : k_q;
    for (u16 i = 0; i < n; ++i) {
        const u16 j = static_cast<u16>((m_hp + k_hist - 1u - i) % k_hist);
        if (m_hx[j] == x && m_hy[j] == y) {
            return true;
        }
    }
    return false;
}

bool RiverExploreMk2::blk_peek (u16 x, u16 y) const {
    return hist8_has(x, y) || bck_has(x, y);
}

bool RiverExploreMk2::is_riv (u16 x, u16 y) const {
    return m_map.get_river(x, y) != 0u;
}

bool RiverExploreMk2::has_unexp_chb (u16 x, u16 y) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(x) + dx;
            const i32 yi = static_cast<i32>(y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(xi);
            const u16 ay = static_cast<u16>(yi);
            if (ax >= w || ay >= h) {
                continue;
            }
            if (m_exp.get(ax, ay) == 0u) {
                return true;
            }
        }
    }
    return false;
}

bool RiverExploreMk2::is_riv_front (u16 x, u16 y) const {
    return is_riv(x, y) && has_unexp_chb(x, y);
}

bool RiverExploreMk2::has_pick (u16 x, u16 y) const {
    u16 nx = 0;
    u16 ny = 0;
    return pick_near_front(x, y, nx, ny);
}

bool RiverExploreMk2::pick_near_front (u16 sx, u16 sy, u16& ox, u16& oy) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const bool use_blk = m_has_blk && m_ph == k_ph_greedy;
    const bool use4 = m_ph == k_ph_greedy;
    for (u32 k = 0; k < (use4 ? 4u : 8u); ++k) {
        const i32 nx = static_cast<i32>(sx) + (use4 ? k_dx4[k] : k_dx8[k]);
        const i32 ny = static_cast<i32>(sy) + (use4 ? k_dy4[k] : k_dy8[k]);
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h) {
            continue;
        }
        if (use_blk && cx == m_blk_x && cy == m_blk_y) {
            continue;
        }
        if (bck_has(cx, cy)) {
            continue;
        }
        if (!is_riv_front(cx, cy)) {
            continue;
        }
        ox = cx;
        oy = cy;
        return true;
    }
    return false;
}

bool RiverExploreMk2::peek_branch (u16 jx, u16 jy, u16 sx, u16 sy) const {
    u16 vx[k_pkq];
    u16 vy[k_pkq];
    u16 vn = 0;
    u16 stk_x[k_pkq];
    u16 stk_y[k_pkq];
    u16 sn = 0;
    stk_x[0] = sx;
    stk_y[0] = sy;
    sn = 1u;
    while (sn > 0u) {
        --sn;
        const u16 cx = stk_x[sn];
        const u16 cy = stk_y[sn];
        if (has_unexp_chb(cx, cy)) {
            return true;
        }
        if (vn >= k_pkq) {
            continue;
        }
        bool dup = false;
        for (u16 vi = 0; vi < vn; ++vi) {
            if (vx[vi] == cx && vy[vi] == cy) {
                dup = true;
                break;
            }
        }
        if (dup) {
            continue;
        }
        vx[vn] = cx;
        vy[vn] = cy;
        ++vn;
        const u16 w = m_map.width();
        const u16 h = m_map.height();
        for (u32 k = 0; k < 8u; ++k) {
            const i32 nx = static_cast<i32>(cx) + k_dx8[k];
            const i32 ny = static_cast<i32>(cy) + k_dy8[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(nx);
            const u16 ay = static_cast<u16>(ny);
            if (ax >= w || ay >= h || !is_riv(ax, ay)) {
                continue;
            }
            if (ax == jx && ay == jy) {
                continue;
            }
            if (blk_peek(ax, ay)) {
                continue;
            }
            bool seen = false;
            for (u16 vi = 0; vi < vn; ++vi) {
                if (vx[vi] == ax && vy[vi] == ay) {
                    seen = true;
                    break;
                }
            }
            if (seen) {
                continue;
            }
            if (vn >= k_pkq || sn >= k_pkq) {
                continue;
            }
            stk_x[sn] = ax;
            stk_y[sn] = ay;
            ++sn;
        }
    }
    return false;
}

bool RiverExploreMk2::try_peek_branch (bool cont) {
    const u16 jx = m_x;
    const u16 jy = m_y;
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0; k < 8u; ++k) {
        const i32 nx = static_cast<i32>(jx) + k_dx8[k];
        const i32 ny = static_cast<i32>(jy) + k_dy8[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h || !is_riv(cx, cy)) {
            continue;
        }
        if (cont && m_has_blk && cx == m_blk_x && cy == m_blk_y) {
            continue;
        }
        if (blk_peek(cx, cy)) {
            continue;
        }
        if (!peek_branch(jx, jy, cx, cy)) {
            continue;
        }
        const u16 ox = jx;
        const u16 oy = jy;
        hist_push(ox, oy);
        m_x = cx;
        m_y = cy;
        reveal_delta(ox, oy, m_x, m_y);
        m_blk_x = ox;
        m_blk_y = oy;
        m_has_blk = true;
        if (cont) {
            m_ph = k_ph_greedy;
            m_greedy_ph = true;
        } else {
            start_greedy_ph();
            m_blk_x = ox;
            m_blk_y = oy;
            m_has_blk = true;
        }
        return true;
    }
    return false;
}

void RiverExploreMk2::start_greedy_ph () {
    m_ph = k_ph_greedy;
    m_greedy_ph = true;
    m_has_blk = false;
    bck_clr();
    gs_push(m_x, m_y);
}

bool RiverExploreMk2::try_resume_greedy () {
    if (!has_pick(m_x, m_y)) {
        return false;
    }
    start_greedy_ph();
    return true;
}

void RiverExploreMk2::end_greedy_local () {
    m_greedy_ph = false;
    m_has_blk = false;
    ge_push(m_x, m_y);
    if (m_hn == 0u) {
        if (!try_resume_greedy() && !try_peek_branch(false)) {
            end_all();
        }
        return;
    }
    bck_push(m_x, m_y);
    m_ph = k_ph_back;
}

void RiverExploreMk2::end_all () {
    m_greedy_ph = false;
    m_has_blk = false;
    bck_clr();
    m_ph = k_ph_done;
}

void RiverExploreMk2::reveal_around () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(m_x) + dx;
            const i32 yi = static_cast<i32>(m_y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h || m_exp.get(x, y) != 0u) {
                continue;
            }
            m_exp.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(m_player)));
        }
    }
}

void RiverExploreMk2::reveal_delta (u16 ox, u16 oy, u16 nx, u16 ny) {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(nx) + dx;
            const i32 yi = static_cast<i32>(ny) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h) {
                continue;
            }
            const i32 oxo = static_cast<i32>(x) - static_cast<i32>(ox);
            const i32 oyo = static_cast<i32>(y) - static_cast<i32>(oy);
            const i32 oax = (oxo < 0) ? -oxo : oxo;
            const i32 oay = (oyo < 0) ? -oyo : oyo;
            if (static_cast<u32>((oax > oay) ? oax : oay) <= static_cast<u32>(m_sight)) {
                continue;
            }
            if (m_exp.get(x, y) != 0u) {
                continue;
            }
            m_exp.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(m_player)));
        }
    }
}

bool RiverExploreMk2::step_greedy () {
    if (!m_greedy_ph) {
        return false;
    }
    u16 nx = 0;
    u16 ny = 0;
    if (pick_near_front(m_x, m_y, nx, ny)) {
        const u16 ox = m_x;
        const u16 oy = m_y;
        hist_push(ox, oy);
        m_blk_x = ox;
        m_blk_y = oy;
        m_has_blk = true;
        m_x = nx;
        m_y = ny;
        reveal_delta(ox, oy, m_x, m_y);
        return true;
    }
    if (try_peek_branch(true)) {
        return true;
    }
    end_greedy_local();
    return false;
}

bool RiverExploreMk2::step_back () {
    if (try_resume_greedy()) {
        return false;
    }
    if (try_peek_branch(false)) {
        return false;
    }
    u16 tx = 0;
    u16 ty = 0;
    if (!hist_pop(tx, ty)) {
        end_all();
        return false;
    }
    const u16 ox = m_x;
    const u16 oy = m_y;
    m_x = tx;
    m_y = ty;
    reveal_delta(ox, oy, m_x, m_y);
    bck_push(ox, oy);
    if (try_resume_greedy()) {
        return false;
    }
    if (try_peek_branch(false)) {
        return false;
    }
    if (m_hn == 0u) {
        end_all();
        return false;
    }
    return true;
}

void RiverExploreMk2::move (u16 moves) {
    if (m_ph == k_ph_done) {
        return;
    }
    reveal_around();
    for (u16 m = 0; m < moves; ++m) {
        if (m_ph == k_ph_done) {
            break;
        }
        bool acted = false;
        u16 loop = 0u;
        while (!acted && m_ph != k_ph_done && loop < k_hist) {
            ++loop;
            if (m_ph == k_ph_back) {
                acted = step_back();
            } else if (m_ph == k_ph_greedy) {
                acted = step_greedy();
            } else {
                break;
            }
        }
        if (loop >= k_hist && m_ph != k_ph_done) {
            end_all();
        }
    }
}

u8 RiverExploreMk2::phase () const {
    return m_ph;
}

u16 RiverExploreMk2::x () const {
    return m_x;
}

u16 RiverExploreMk2::y () const {
    return m_y;
}

u16 RiverExploreMk2::gre_end_n () const {
    return m_ge_n;
}

u16 RiverExploreMk2::gre_end_x (u16 i) const {
    return (i < m_ge_n) ? m_gex[i] : 0u;
}

u16 RiverExploreMk2::gre_end_y (u16 i) const {
    return (i < m_ge_n) ? m_gey[i] : 0u;
}

u16 RiverExploreMk2::gre_start_n () const {
    return m_gs_n;
}

u16 RiverExploreMk2::gre_start_x (u16 i) const {
    return (i < m_gs_n) ? m_gsx[i] : 0u;
}

u16 RiverExploreMk2::gre_start_y (u16 i) const {
    return (i < m_gs_n) ? m_gsy[i] : 0u;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
