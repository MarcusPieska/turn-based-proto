//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_coast_mk1.h"
#include "game_map_defs.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const i16 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i16 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const i16 k_dx4[4] = {0, 1, 0, -1};
static const i16 k_dy4[4] = {-1, 0, 1, 0}; 

//================================================================================================================================
//=> - WalkCoastMk1 -
//================================================================================================================================

WalkCoastMk1::WalkCoastMk1 (
    const GameArraySimple& map,
    u8* exp,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player) :
    m_map(map),
    m_exp(exp),
    m_x(sx),
    m_y(sy),
    m_ax(sx),
    m_ay(sy),
    m_sight(sight),
    m_player(player),
    m_ph(k_ph_walk),
    m_done(false),
    m_loc_exh(false) {
    reveal_around(m_x, m_y);
}

u32 WalkCoastMk1::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_map.width()) + static_cast<u32>(x);
}

u16 WalkCoastMk1::lidx (u16 lx, u16 ly) const {
    return static_cast<u16>(ly * k_box + lx);
}

bool WalkCoastMk1::g2l (u16 gx, u16 gy, u16& lx, u16& ly) const {
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

bool WalkCoastMk1::l2g (u16 lx, u16 ly, u16& gx, u16& gy) const {
    const i32 gx_i = static_cast<i32>(lx) - static_cast<i32>(k_cen) + static_cast<i32>(m_ax);
    const i32 gy_i = static_cast<i32>(ly) - static_cast<i32>(k_cen) + static_cast<i32>(m_ay);
    if (gx_i < 0 || gy_i < 0) {
        return false;
    }
    gx = static_cast<u16>(gx_i);
    gy = static_cast<u16>(gy_i);
    return true;
}

bool WalkCoastMk1::is_coast (u16 x, u16 y) const {
    return m_map.get_terrain(x, y) == TERR_COASTAL[0];
}

bool WalkCoastMk1::is_path (u16 x, u16 y) const {
    const u8 t = m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0] || t == TERR_TILE_SENTINEL[0]) {
        return false;
    }
    return true;
}

bool WalkCoastMk1::has_coast_nbr (u16 x, u16 y) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0u; k < 4u; ++k) {
        const i32 xi = static_cast<i32>(x) + k_dx4[k];
        const i32 yi = static_cast<i32>(y) + k_dy4[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(xi);
        const u16 ay = static_cast<u16>(yi);
        if (ax >= w || ay >= h) {
            continue;
        }
        if (is_coast(ax, ay)) {
            return true;
        }
    }
    return false;
}

bool WalkCoastMk1::is_walk (u16 x, u16 y) const {
    return is_path(x, y) && has_coast_nbr(x, y);
}

u32 WalkCoastMk1::cnt_new (u16 x, u16 y) const {
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

bool WalkCoastMk1::has_pick () const {
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

void WalkCoastMk1::reveal_around (u16 x, u16 y) {
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
        }
    }
}

bool WalkCoastMk1::build_grad () {
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
            if (!l2g(lx, ly, gx, gy) || gx >= w || gy >= h || !is_path(gx, gy)) {
                m_dst[i] = k_blk;
                continue;
            }
            if (m_exp[tidx(gx, gy)] != 0u) {
                m_dst[i] = k_non;
                continue;
            }
            if (has_coast_nbr(gx, gy)) {
                m_dst[i] = 0u;
                fog = true;
                q[qt] = i;
                ++qt;
            } else {
                m_dst[i] = k_wat;
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
            if (m_dst[ni] != k_wat && m_dst[ni] != k_non) {
                continue;
            }
            u16 ngx = 0u;
            u16 ngy = 0u;
            if (!l2g(nlx, nly, ngx, ngy) || !is_path(ngx, ngy)) {
                continue;
            }
            m_dst[ni] = static_cast<u8>(cd + 1u);
            q[qt] = ni;
            ++qt;
        }
    }
    return true;
}

bool WalkCoastMk1::step_walk () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    u32 best = 0u;
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
        const u32 t = cnt_new(nx, ny);
        if (t == 0u) {
            continue;
        }
        if (t > best) {
            best = t;
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

bool WalkCoastMk1::step_repos () {
    u16 lx = 0u;
    u16 ly = 0u;
    if (!g2l(m_x, m_y, lx, ly)) {
        return false;
    }
    const u16 ci = lidx(lx, ly);
    const u8 cur = m_dst[ci];
    if (cur >= k_wat) {
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
        if (nd >= k_wat || nd >= cur) {
            continue;
        }
        u16 gx = 0u;
        u16 gy = 0u;
        if (!l2g(nlx, nly, gx, gy) || !is_path(gx, gy)) {
            continue;
        }
        if (nd < best) {
            best = nd;
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

bool WalkCoastMk1::step () {
    if (m_done) {
        return false;
    }
    if (has_pick()) {
        m_ph = k_ph_walk;
        return step_walk();
    }
    if (m_ph == k_ph_walk) {
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

void WalkCoastMk1::move (u16 moves) {
    for (u16 m = 0u; m < moves && !m_done; ++m) {
        if (!step()) {
            break;
        }
    }
}

u16 WalkCoastMk1::x () const {
    return m_x;
}

u16 WalkCoastMk1::y () const {
    return m_y;
}

u8 WalkCoastMk1::phase () const {
    return m_ph;
}

bool WalkCoastMk1::done () const {
    return m_done;
}

bool WalkCoastMk1::loc_exh () const {
    return m_loc_exh;
}

u8 WalkCoastMk1::grad_max () const {
    if (m_ph != k_ph_repos) {
        return 0u;
    }
    u8 mx = 0u;
    for (u16 i = 0u; i < k_n; ++i) {
        const u8 v = m_dst[i];
        if (v >= k_wat) {
            continue;
        }
        if (v > mx) {
            mx = v;
        }
    }
    return mx;
}

bool WalkCoastMk1::grad_tile (u16 gi, u16& x, u16& y, u8& d) const {
    if (m_ph != k_ph_repos) {
        return false;
    }
    u16 n = 0u;
    for (u16 ly = 0u; ly < k_box; ++ly) {
        for (u16 lx = 0u; lx < k_box; ++lx) {
            const u8 v = m_dst[lidx(lx, ly)];
            if (v >= k_wat) {
                continue;
            }
            if (n == gi) {
                d = v;
                return l2g(lx, ly, x, y);
            }
            ++n;
        }
    }
    return false;
}

bool WalkCoastMk1::sink_at (u16 si, u16& x, u16& y) const {
    if (m_ph != k_ph_repos) {
        return false;
    }
    u16 n = 0u;
    for (u16 ly = 0u; ly < k_box; ++ly) {
        for (u16 lx = 0u; lx < k_box; ++lx) {
            if (m_dst[lidx(lx, ly)] != 0u) {
                continue;
            }
            if (n == si) {
                return l2g(lx, ly, x, y);
            }
            ++n;
        }
    }
    return false;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
