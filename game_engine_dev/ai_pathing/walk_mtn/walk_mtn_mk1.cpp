//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_mtn_mk1.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "runtime_trace_dbg.h" 

//================================================================================================================================
//=> - WalkMtnMk1 static helpers -
//================================================================================================================================

static bool s_is_mtn (const GameArraySimple& map, u16 x, u16 y) {
    return map.get_terrain(x, y) == TERR_MOUNTAINS[0];
}

static bool s_is_path (const GameArraySimple& map, u16 x, u16 y) {
    const u8 t = map.get_terrain(x, y);
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

static bool s_has_mtn_nbr (const GameArraySimple& map, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
        const i32 xi = static_cast<i32>(x) + MAP_NBR4_DX[k];
        const i32 yi = static_cast<i32>(y) + MAP_NBR4_DY[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(xi);
        const u16 ay = static_cast<u16>(yi);
        if (ax >= w || ay >= h) {
            continue;
        }
        if (s_is_mtn(map, ax, ay)) {
            return true;
        }
    }
    return false;
}

static bool s_is_walk (const GameArraySimple& map, u16 x, u16 y) {
    return s_is_path(map, x, y) && s_has_mtn_nbr(map, x, y);
}

static u32 s_cnt_new (const GameArraySimple& map, MapBitOverlay& exp, u16 sight, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    u32 n = 0u;
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(sight)) {
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
            if (exp.get(ax, ay) != 0u) {
                continue;
            }
            ++n;
        }
    }
    return n;
}

static bool s_has_pick (const GameArraySimple& map, MapBitOverlay& exp, u16 sight, u16 x, u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 xi = static_cast<i32>(x) + MAP_NBR8_DX[k];
        const i32 yi = static_cast<i32>(y) + MAP_NBR8_DY[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 nx = static_cast<u16>(xi);
        const u16 ny = static_cast<u16>(yi);
        if (nx >= w || ny >= h) {
            continue;
        }
        if (!s_is_walk(map, nx, ny)) {
            continue;
        }
        if (s_cnt_new(map, exp, sight, nx, ny) > 0u) {
            return true;
        }
    }
    return false;
}

static bool s_l2g (u16 ax, u16 ay, u16 lx, u16 ly, u16& gx, u16& gy) {
    const i32 gx_i = static_cast<i32>(lx) - 15 + static_cast<i32>(ax);
    const i32 gy_i = static_cast<i32>(ly) - 15 + static_cast<i32>(ay);
    if (gx_i < 0 || gy_i < 0) {
        return false;
    }
    gx = static_cast<u16>(gx_i);
    gy = static_cast<u16>(gy_i);
    return true;
}

static bool s_grad_has_fog (const GameArraySimple& map, MapBitOverlay& exp, u16 ax, u16 ay) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 ly = 0u; ly < 31u; ++ly) {
        for (u16 lx = 0u; lx < 31u; ++lx) {
            u16 gx = 0u;
            u16 gy = 0u;
            if (!s_l2g(ax, ay, lx, ly, gx, gy) || gx >= w || gy >= h || !s_is_path(map, gx, gy)) {
                continue;
            }
            if (exp.get(gx, gy) != 0u) {
                continue;
            }
            if (s_has_mtn_nbr(map, gx, gy)) {
                return true;
            }
        }
    }
    return false;
}

static bool s_grad_reachable (const GameArraySimple& map, MapBitOverlay& exp, u16 ax, u16 ay) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 k_box = 31u;
    const u8 k_blk = 255u;
    const u8 k_non = 254u;
    const u8 k_wat = 253u;
    u8 dst[961];
    u16 q[961];
    u16 qh = 0u;
    u16 qt = 0u;
    bool fog = false;
    for (u16 ly = 0u; ly < k_box; ++ly) {
        for (u16 lx = 0u; lx < k_box; ++lx) {
            const u16 i = static_cast<u16>(ly * k_box + lx);
            u16 gx = 0u;
            u16 gy = 0u;
            if (!s_l2g(ax, ay, lx, ly, gx, gy) || gx >= w || gy >= h || !s_is_path(map, gx, gy)) {
                dst[i] = k_blk;
                continue;
            }
            if (exp.get(gx, gy) != 0u) {
                dst[i] = k_non;
                continue;
            }
            if (s_has_mtn_nbr(map, gx, gy)) {
                dst[i] = 0u;
                fog = true;
                q[qt++] = i;
            } else {
                dst[i] = k_wat;
            }
        }
    }
    if (!fog) {
        return false;
    }
    while (qh < qt) {
        const u16 ci = q[qh++];
        const u16 clx = static_cast<u16>(ci % k_box);
        const u16 cly = static_cast<u16>(ci / k_box);
        const u8 cd = dst[ci];
        for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
            const i32 nxi = static_cast<i32>(clx) + MAP_NBR8_DX[k];
            const i32 nyi = static_cast<i32>(cly) + MAP_NBR8_DY[k];
            if (nxi < 0 || nyi < 0) {
                continue;
            }
            const u16 nlx = static_cast<u16>(nxi);
            const u16 nly = static_cast<u16>(nyi);
            if (nlx >= k_box || nly >= k_box) {
                continue;
            }
            const u16 ni = static_cast<u16>(nly * k_box + nlx);
            if (dst[ni] != k_wat && dst[ni] != k_non) {
                continue;
            }
            u16 ngx = 0u;
            u16 ngy = 0u;
            if (!s_l2g(ax, ay, nlx, nly, ngx, ngy) || !s_is_path(map, ngx, ngy)) {
                continue;
            }
            dst[ni] = static_cast<u8>(cd + 1u);
            q[qt++] = ni;
        }
    }
    const u16 ci = static_cast<u16>(15u * k_box + 15u);
    return dst[ci] < k_wat;
}

u16 WalkMtnMk1::can_ai_start_from (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 x,
    u16 y,
    u16 sight) {
    if (!s_is_path(map, x, y)) {
        return 0u;
    }
    if (s_is_walk(map, x, y) && s_has_pick(map, exp, sight, x, y)) {
        return static_cast<u16>(k_ph_walk) + 1u;
    }
    if (s_grad_has_fog(map, exp, x, y) && s_grad_reachable(map, exp, x, y)) {
        return static_cast<u16>(k_ph_repos) + 1u;
    }
    return 0u;
}

//================================================================================================================================
//=> - WalkMtnMk1 -
//================================================================================================================================

WalkMtnMk1::WalkMtnMk1 (const GameArraySimple& map, MapBitOverlay& exp, u16 sx, u16 sy, u16 sight, u8 player) :
    m_map(map),
    m_exp(exp),
    m_pos{sx, sy},
    m_win{sx, sy},
    m_sight(sight),
    m_player(player),
    m_done(false),
    m_loc_exh(false) {
    const u16 st = can_ai_start_from(map, exp, sx, sy, sight);
    if (st == static_cast<u16>(k_ph_repos) + 1u) {
        m_ph = k_ph_repos;
    } else if (st == static_cast<u16>(k_ph_walk) + 1u) {
        m_ph = k_ph_walk;
    } else {
        m_ph = k_ph_walk;
    }
    if (m_ph == k_ph_repos) {
        build_grad();
    }
    reveal_around(m_pos.x, m_pos.y);
}

u32 WalkMtnMk1::tidx (u16 x, u16 y) const {
    return static_cast<u32>(y) * static_cast<u32>(m_map.width()) + static_cast<u32>(x);
}

u16 WalkMtnMk1::lidx (u16 lx, u16 ly) const {
    return static_cast<u16>(ly * k_box + lx);
}

bool WalkMtnMk1::g2l (u16 gx, u16 gy, u16& lx, u16& ly) const {
    const i32 lx_i = static_cast<i32>(gx) - static_cast<i32>(m_win.x) + static_cast<i32>(k_cen);
    const i32 ly_i = static_cast<i32>(gy) - static_cast<i32>(m_win.y) + static_cast<i32>(k_cen);
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

bool WalkMtnMk1::l2g (u16 lx, u16 ly, u16& gx, u16& gy) const {
    const i32 gx_i = static_cast<i32>(lx) - static_cast<i32>(k_cen) + static_cast<i32>(m_win.x);
    const i32 gy_i = static_cast<i32>(ly) - static_cast<i32>(k_cen) + static_cast<i32>(m_win.y);
    if (gx_i < 0 || gy_i < 0) {
        return false;
    }
    gx = static_cast<u16>(gx_i);
    gy = static_cast<u16>(gy_i);
    return true;
}

bool WalkMtnMk1::is_mtn (u16 x, u16 y) const {
    return m_map.get_terrain(x, y) == TERR_MOUNTAINS[0];
}

bool WalkMtnMk1::is_path (u16 x, u16 y) const {
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

bool WalkMtnMk1::has_mtn_nbr (u16 x, u16 y) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0u; k < MAP_NBR4_N; ++k) {
        const i32 xi = static_cast<i32>(x) + MAP_NBR4_DX[k];
        const i32 yi = static_cast<i32>(y) + MAP_NBR4_DY[k];
        if (xi < 0 || yi < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(xi);
        const u16 ay = static_cast<u16>(yi);
        if (ax >= w || ay >= h) {
            continue;
        }
        if (is_mtn(ax, ay)) {
            return true;
        }
    }
    return false;
}

bool WalkMtnMk1::is_walk (u16 x, u16 y) const {
    return is_path(x, y) && has_mtn_nbr(x, y);
}

u32 WalkMtnMk1::cnt_new (u16 x, u16 y) const {
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
            if (m_exp.get(ax, ay) != 0u) {
                continue;
            }
            ++n;
        }
    }
    return n;
}

bool WalkMtnMk1::has_pick () const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 xi = static_cast<i32>(m_pos.x) + MAP_NBR8_DX[k];
        const i32 yi = static_cast<i32>(m_pos.y) + MAP_NBR8_DY[k];
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

void WalkMtnMk1::reveal_around (u16 x, u16 y) {
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
            if (m_exp.get(ax, ay) != 0u) {
                continue;
            }
            m_exp.set(ax, ay);
            TRACE_EXPLORE_DISCOVER((ax, ay, static_cast<u16>(m_player)));
        }
    }
}

bool WalkMtnMk1::build_grad () {
    m_win = m_pos;
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
            if (m_exp.get(gx, gy) != 0u) {
                m_dst[i] = k_non;
                continue;
            }
            if (has_mtn_nbr(gx, gy)) {
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
        for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
            const i32 nxi = static_cast<i32>(clx) + MAP_NBR8_DX[k];
            const i32 nyi = static_cast<i32>(cly) + MAP_NBR8_DY[k];
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

bool WalkMtnMk1::step_walk () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    u32 best = 0u;
    u16 bx = m_pos.x;
    u16 by = m_pos.y;
    bool found = false;
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 xi = static_cast<i32>(m_pos.x) + MAP_NBR8_DX[k];
        const i32 yi = static_cast<i32>(m_pos.y) + MAP_NBR8_DY[k];
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
    m_pos.x = bx;
    m_pos.y = by;
    reveal_around(m_pos.x, m_pos.y);
    return true;
}

bool WalkMtnMk1::step_repos () {
    u16 lx = 0u;
    u16 ly = 0u;
    if (!g2l(m_pos.x, m_pos.y, lx, ly)) {
        return false;
    }
    const u16 ci = lidx(lx, ly);
    const u8 cur = m_dst[ci];
    if (cur >= k_wat) {
        return false;
    }
    u8 best = cur;
    u16 bx = m_pos.x;
    u16 by = m_pos.y;
    bool found = false;
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 nxi = static_cast<i32>(lx) + MAP_NBR8_DX[k];
        const i32 nyi = static_cast<i32>(ly) + MAP_NBR8_DY[k];
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
    m_pos.x = bx;
    m_pos.y = by;
    reveal_around(m_pos.x, m_pos.y);
    return true;
}

bool WalkMtnMk1::step () {
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

void WalkMtnMk1::move (u16 moves) {
    for (u16 m = 0u; m < moves && !m_done; ++m) {
        if (!step()) {
            break;
        }
    }
}

u16 WalkMtnMk1::x () const {
    return m_pos.x;
}

u16 WalkMtnMk1::y () const {
    return m_pos.y;
}

u8 WalkMtnMk1::phase () const {
    return m_ph;
}

bool WalkMtnMk1::done () const {
    return m_done;
}

bool WalkMtnMk1::loc_exh () const {
    return m_loc_exh;
}

u8 WalkMtnMk1::grad_max () const {
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

bool WalkMtnMk1::grad_tile (u16 gi, u16& x, u16& y, u8& d) const {
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

bool WalkMtnMk1::sink_at (u16 si, u16& x, u16& y) const {
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
