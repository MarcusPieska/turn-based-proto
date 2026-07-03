//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "near_explore_mk1.h"
#include "game_map_defs.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const i16 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i16 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

//================================================================================================================================
//=> - NearExploreMk1 -
//================================================================================================================================

NearExploreMk1::NearExploreMk1 (
    const GameArraySimple& map,
    u8* exp,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player,
    NearExploreBias bias) :
    m_map(map),
    m_exp(exp),
    m_x(sx),
    m_y(sy),
    m_hx(sx),
    m_hy(sy),
    m_ax(sx),
    m_ay(sy),
    m_sight(sight),
    m_player(player),
    m_bias(static_cast<u8>(bias)),
    m_ph(k_ph_explore),
    m_done(false),
    m_loc_exh(false),
    m_riv_spot(false),
    m_riv_x(0),
    m_riv_y(0) {
    reveal_around(m_x, m_y);
}

u32 NearExploreMk1::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_map.width()) + static_cast<u32>(x);
}

u16 NearExploreMk1::lidx (u16 lx, u16 ly) const {
    return static_cast<u16>(ly * k_box + lx);
}

bool NearExploreMk1::g2l (u16 gx, u16 gy, u16& lx, u16& ly) const {
    const i32 lx_i = static_cast<i32>(gx) - static_cast<i32>(m_ax) + static_cast<i32>(k_cen);
    const i32 ly_i = static_cast<i32>(gy) - static_cast<i32>(m_ay) + static_cast<i32>(k_cen);
    if (lx_i < 0 || ly_i < 0) {
        return false;
    }
    if (lx_i >= static_cast<i32>(k_box) || ly_i >= static_cast<i32>(k_box)) {
        return false;
    }
    lx = static_cast<u16>(lx_i);
    ly = static_cast<u16>(ly_i);
    return true;
}

bool NearExploreMk1::l2g (u16 lx, u16 ly, u16& gx, u16& gy) const {
    const i32 gx_i = static_cast<i32>(lx) - static_cast<i32>(k_cen) + static_cast<i32>(m_ax);
    const i32 gy_i = static_cast<i32>(ly) - static_cast<i32>(k_cen) + static_cast<i32>(m_ay);
    if (gx_i < 0 || gy_i < 0) {
        return false;
    }
    gx = static_cast<u16>(gx_i);
    gy = static_cast<u16>(gy_i);
    return true;
}

bool NearExploreMk1::is_walk (u16 x, u16 y) const {
    const u8 t = m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0]) {
        return false;
    }
    return true;
}

u32 NearExploreMk1::cheb (u16 x0, u16 y0, u16 x1, u16 y1) const {
    const u32 dx = (x0 > x1) ? static_cast<u32>(x0 - x1) : static_cast<u32>(x1 - x0);
    const u32 dy = (y0 > y1) ? static_cast<u32>(y0 - y1) : static_cast<u32>(y1 - y0);
    return (dx > dy) ? dx : dy;
}

u32 NearExploreMk1::adj_dist (u16 x, u16 y) const {
    u32 d = cheb(x, y, m_hx, m_hy);
    if (m_bias == NE_BIAS_NONE) {
        return d;
    }
    if (m_bias == NE_BIAS_NORTH) {
        if (y < m_hy) {
            d /= 4u;
        } else if (y > m_hy) {
            d *= 4u;
        }
        return d;
    }
    if (m_bias == NE_BIAS_SOUTH) {
        if (y > m_hy) {
            d /= 4u;
        } else if (y < m_hy) {
            d *= 4u;
        }
        return d;
    }
    if (m_bias == NE_BIAS_EAST) {
        if (x > m_hx) {
            d /= 4u;
        } else if (x < m_hx) {
            d *= 4u;
        }
        return d;
    }
    if (x < m_hx) {
        d /= 4u;
    } else if (x > m_hx) {
        d *= 4u;
    }
    return d;
}

u32 NearExploreMk1::cnt_new (u16 x, u16 y) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    u32 n = 0u;
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
            if (m_exp[tidx(ax, ay)] != 0u) {
                continue;
            }
            ++n;
        }
    }
    return n;
}

bool NearExploreMk1::has_pick () const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0u; k < 8u; ++k) {
        const i32 xi = static_cast<i32>(m_x) + k_dx8[k];
        const i32 yi = static_cast<i32>(m_y) + k_dy8[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 nx = static_cast<u16>(xi);
        const u16 ny = static_cast<u16>(yi);
        if (nx >= w || ny >= h) {
            continue;
        }
        if (!is_walk(nx, ny)) {
            continue;
        }
        if (cnt_new(nx, ny) > 0u) {
            return true;
        }
    }
    return false;
}

i32 NearExploreMk1::score (u16 x, u16 y) const {
    u32 t = cnt_new(x, y);
    if (t == 0u) {
        return -0x7FFFFFFFL;
    }
    t = (t < k_l) ? t : k_l;
    u32 d = adj_dist(x, y);
    return static_cast<i32>(t) - static_cast<i32>(d + d + d);
}

bool NearExploreMk1::is_riv (u16 x, u16 y) const {
    return m_map.get_river(x, y) != 0u;
}

void NearExploreMk1::note_riv (u16 ax, u16 ay) {
    if (!is_riv(ax, ay)) {
        return;
    }
    const u32 d = cheb(ax, ay, m_x, m_y);
    if (!m_riv_spot || d < cheb(m_riv_x, m_riv_y, m_x, m_y)) {
        m_riv_spot = true;
        m_riv_x = ax;
        m_riv_y = ay;
    }
}

bool NearExploreMk1::build_grad () {
    m_ax = m_x;
    m_ay = m_y;
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    u16 q[k_n];
    u16 qh = 0u;
    u16 qt = 0u;
    bool fog = false;
    for (u16 ly = 0u; ly < k_box; ++ly) {
        for (u16 lx = 0u; lx < k_box; ++lx) {
            const u16 i = lidx(lx, ly);
            u16 gx = 0u;
            u16 gy = 0u;
            if (!l2g(lx, ly, gx, gy) || gx >= w || gy >= h || !is_walk(gx, gy)) {
                m_dst[i] = k_blk;
                continue;
            }
            if (m_exp[tidx(gx, gy)] == 0u) {
                m_dst[i] = 0u;
                fog = true;
                q[qt] = i;
                ++qt;
            } else {
                m_dst[i] = k_non;
            }
        }
    }
    if (!fog) {
        return false;
    }
    while (qh < qt) {
        const u16 ci = q[qh];
        ++qh;
        const u16 clx = static_cast<u16>(ci % k_box);
        const u16 cly = static_cast<u16>(ci / k_box);
        const u8 cd = m_dst[ci];
        for (u32 k = 0u; k < 8u; ++k) {
            const i32 nxi = static_cast<i32>(clx) + k_dx8[k];
            const i32 nyi = static_cast<i32>(cly) + k_dy8[k];
            if (nxi < 0 || nyi < 0) {
                continue;
            }
            const u16 nlx = static_cast<u16>(nxi);
            const u16 nly = static_cast<u16>(nyi);
            if (nlx >= k_box || nly >= k_box) {
                continue;
            }
            const u16 ni = lidx(nlx, nly);
            if (m_dst[ni] != k_non) {
                continue;
            }
            m_dst[ni] = static_cast<u8>(cd + 1u);
            q[qt] = ni;
            ++qt;
        }
    }
    return true;
}

bool NearExploreMk1::step_explore () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    i32 best = -0x7FFFFFFFL;
    u16 bx = m_x;
    u16 by = m_y;
    bool found = false;
    for (u32 k = 0u; k < 8u; ++k) {
        const i32 xi = static_cast<i32>(m_x) + k_dx8[k];
        const i32 yi = static_cast<i32>(m_y) + k_dy8[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 nx = static_cast<u16>(xi);
        const u16 ny = static_cast<u16>(yi);
        if (nx >= w || ny >= h) {
            continue;
        }
        if (!is_walk(nx, ny)) {
            continue;
        }
        const i32 sc = score(nx, ny);
        if (sc > best) {
            best = sc;
            bx = nx;
            by = ny;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    m_x = bx;
    m_y = by;
    reveal_around(m_x, m_y);
    return true;
}

bool NearExploreMk1::step_repos () {
    u16 lx = 0u;
    u16 ly = 0u;
    if (!g2l(m_x, m_y, lx, ly)) {
        return false;
    }
    const u16 ci = lidx(lx, ly);
    const u8 cur = m_dst[ci];
    if (cur == k_blk || cur == k_non) {
        return false;
    }
    u8 best = cur;
    u16 bx = m_x;
    u16 by = m_y;
    bool found = false;
    for (u32 k = 0u; k < 8u; ++k) {
        const i32 nxi = static_cast<i32>(lx) + k_dx8[k];
        const i32 nyi = static_cast<i32>(ly) + k_dy8[k];
        if (nxi < 0 || nyi < 0) {
            continue;
        }
        const u16 nlx = static_cast<u16>(nxi);
        const u16 nly = static_cast<u16>(nyi);
        if (nlx >= k_box || nly >= k_box) {
            continue;
        }
        const u8 nd = m_dst[lidx(nlx, nly)];
        if (nd >= cur || nd == k_blk || nd == k_non) {
            continue;
        }
        if (nd < best) {
            best = nd;
            u16 gx = 0u;
            u16 gy = 0u;
            if (!l2g(nlx, nly, gx, gy)) {
                continue;
            }
            bx = gx;
            by = gy;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    m_x = bx;
    m_y = by;
    reveal_around(m_x, m_y);
    return true;
}

bool NearExploreMk1::step () {
    if (m_done) {
        return false;
    }
    if (has_pick()) {
        m_ph = k_ph_explore;
        return step_explore();
    }
    if (m_ph == k_ph_explore) {
        if (!build_grad()) {
            m_loc_exh = true;
            m_done = true;
            return false;
        }
        m_ph = k_ph_repos;
    }
    if (step_repos()) {
        return true;
    }
    if (build_grad() && step_repos()) {
        return true;
    }
    m_loc_exh = true;
    m_done = true;
    return false;
}

void NearExploreMk1::reveal_around (u16 x, u16 y) {
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
            if (m_exp[tidx(ax, ay)] != 0u) {
                continue;
            }
            m_exp[tidx(ax, ay)] = 1u;
            TRACE_EXPLORE_DISCOVER((ax, ay, static_cast<u16>(m_player)));
            note_riv(ax, ay);
        }
    }
}

void NearExploreMk1::move (u16 moves) {
    for (u16 m = 0u; m < moves && !m_done; ++m) {
        if (!step()) {
            break;
        }
    }
}

u16 NearExploreMk1::x () const {
    return m_x;
}

u16 NearExploreMk1::y () const {
    return m_y;
}

u16 NearExploreMk1::hx () const {
    return m_hx;
}

u16 NearExploreMk1::hy () const {
    return m_hy;
}

u8 NearExploreMk1::phase () const {
    return m_ph;
}

bool NearExploreMk1::done () const {
    return m_done;
}

bool NearExploreMk1::loc_exh () const {
    return m_loc_exh;
}

bool NearExploreMk1::riv_spot () const {
    return m_riv_spot;
}

u16 NearExploreMk1::riv_x () const {
    return m_riv_x;
}

u16 NearExploreMk1::riv_y () const {
    return m_riv_y;
}

void NearExploreMk1::clr_riv_spot () {
    m_riv_spot = false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
