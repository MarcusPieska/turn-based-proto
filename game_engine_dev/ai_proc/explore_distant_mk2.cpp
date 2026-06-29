//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "explore_distant_mk2.h"
#include "game_map_defs.h"
#include "runtime_trace_dbg.h"

#include <cstdio>

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u32 k_dist_none = 0xFFFFFFFFu;
static const u32 k_par_none = 0xFFFFFFFFu;
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

static u32 dist_man (u16 x0, u16 y0, u16 x1, u16 y1) {
    const u32 dx = (x0 > x1) ? static_cast<u32>(x0 - x1) : static_cast<u32>(x1 - x0);
    const u32 dy = (y0 > y1) ? static_cast<u32>(y0 - y1) : static_cast<u32>(y1 - y0);
    return dx + dy;
}

static void path_fail (cstr where, u16 x, u16 y, u16 tx, u16 ty) {
    char buf[72];
    std::snprintf(buf, sizeof(buf), "%s (%u,%u)->(%u,%u)", where,
        static_cast<unsigned>(x), static_cast<unsigned>(y),
        static_cast<unsigned>(tx), static_cast<unsigned>(ty));
    TRACE_PATH_FAILURE((buf));
}

static bool is_frontier (
    const MapBitOverlay& exp,
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u16 x,
    u16 y) {
    if (exp.get(x, y) != 0u || !is_walk(map.get_terrain(x, y))) {
        return false;
    }
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(x) + k_dx4[i];
        const i32 ny = static_cast<i32>(y) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(nx);
        const u16 ay = static_cast<u16>(ny);
        if (ax < w && ay < h && exp.get(ax, ay) != 0u) {
            return true;
        }
    }
    return false;
}

static bool bfs_step (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u32 tile_n,
    u16 ux,
    u16 uy,
    u16 tx,
    u16 ty,
    u16& ox,
    u16& oy);

static bool bfs_can_reach (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u32 tile_n,
    u16 ux,
    u16 uy,
    u16 tx,
    u16 ty) {
    u16 sx = 0;
    u16 sy = 0;
    return bfs_step(map, w, h, tile_n, ux, uy, tx, ty, sx, sy);
}

static bool pick_global (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 ux,
    u16 uy,
    bool has_skip,
    u16 skip_x,
    u16 skip_y,
    u16 w,
    u16 h,
    u32 tile_n,
    u16& ox,
    u16& oy) {
    u32 best = 0xFFFFFFFFu;
    bool found = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (exp.get(x, y) != 0u || !is_walk(map.get_terrain(x, y))) {
                continue;
            }
            if (has_skip && x == skip_x && y == skip_y) {
                continue;
            }
            const u32 d = dist_man(ux, uy, x, y);
            if (d >= best) {
                continue;
            }
            if (!bfs_can_reach(map, w, h, tile_n, ux, uy, x, y)) {
                continue;
            }
            best = d;
            ox = x;
            oy = y;
            found = true;
        }
    }
    return found;
}

static bool bfs_step (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u32 tile_n,
    u16 ux,
    u16 uy,
    u16 tx,
    u16 ty,
    u16& ox,
    u16& oy) {
    u32* par = new u32[tile_n];
    u32* q = new u32[tile_n];
    if (par == nullptr || q == nullptr) {
        delete[] par;
        delete[] q;
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        par[i] = k_par_none;
    }
    const u32 si = tidx(w, ux, uy);
    const u32 gi = tidx(w, tx, ty);
    u32 qn = 0;
    par[si] = si;
    q[qn++] = si;
    bool found = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        if (i == gi) {
            found = true;
            break;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_walk(map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (par[ni] != k_par_none) {
                continue;
            }
            par[ni] = i;
            q[qn++] = ni;
        }
    }
    if (!found) {
        delete[] par;
        delete[] q;
        return false;
    }
    u32 cur = gi;
    while (par[cur] != si) {
        cur = par[cur];
    }
    const u16 py = static_cast<u16>(cur / static_cast<u32>(w));
    ox = static_cast<u16>(cur - static_cast<u32>(py) * static_cast<u32>(w));
    oy = py;
    delete[] par;
    delete[] q;
    return true;
}

//================================================================================================================================
//=> - ExploreDistantMk2 -
//================================================================================================================================

ExploreDistantMk2::ExploreDistantMk2 (
    const GameArraySimple& map,
    MapBitOverlay& explored,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player) :
    ExploreAi(map, explored, sx, sy, sight, player),
    m_tgt_x(0),
    m_tgt_y(0),
    m_skip_x(0),
    m_skip_y(0),
    m_has_tgt(false),
    m_has_skip(false),
    m_done(false) {}

void ExploreDistantMk2::reveal_around () {
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

bool ExploreDistantMk2::pick_frontier () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* dist = new u32[tile_n];
    u32* q = new u32[tile_n];
    if (dist == nullptr || q == nullptr) {
        delete[] dist;
        delete[] q;
        path_fail("pick_frontier oom", m_x, m_y, 0u, 0u);
        return false;
    }
    for (u32 i = 0; i < tile_n; ++i) {
        dist[i] = k_dist_none;
    }
    u32 qn = 0;
    const u32 si = tidx(w, m_x, m_y);
    dist[si] = 0;
    q[qn++] = si;
    u32 best_f = 0xFFFFFFFFu;
    u32 best_a = 0xFFFFFFFFu;
    u16 ax = 0;
    u16 ay = 0;
    m_has_tgt = false;
    bool has_a = false;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u32 d = dist[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (m_exp.get(px, py) == 0u && is_walk(m_map.get_terrain(px, py))) {
            const bool skip = m_has_skip && px == m_skip_x && py == m_skip_y;
            if (!skip && d < best_a) {
                best_a = d;
                ax = px;
                ay = py;
                has_a = true;
            }
            if (!skip && is_frontier(m_exp, m_map, w, h, px, py) && d < best_f) {
                best_f = d;
                m_tgt_x = px;
                m_tgt_y = py;
                m_has_tgt = true;
            }
        }
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_walk(m_map.get_terrain(cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (dist[ni] != k_dist_none) {
                continue;
            }
            dist[ni] = d + 1u;
            q[qn++] = ni;
        }
    }
    delete[] dist;
    delete[] q;
    if (!m_has_tgt && has_a) {
        m_tgt_x = ax;
        m_tgt_y = ay;
        m_has_tgt = true;
    }
    if (!m_has_tgt) {
        u16 gx = 0;
        u16 gy = 0;
        if (pick_global(m_map, m_exp, m_x, m_y, m_has_skip, m_skip_x, m_skip_y, w, h, tile_n, gx, gy)) {
            m_tgt_x = gx;
            m_tgt_y = gy;
            m_has_tgt = true;
        }
    }
    if (!m_has_tgt) {
        path_fail("pick_frontier none", m_x, m_y, 0u, 0u);
    }
    return m_has_tgt;
}

bool ExploreDistantMk2::step_to_tgt () {
    if (m_x == m_tgt_x && m_y == m_tgt_y) {
        return false;
    }
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 cur_d = dist_man(m_x, m_y, m_tgt_x, m_tgt_y);
    u32 best = 0xFFFFFFFFu;
    u16 bx = m_x;
    u16 by = m_y;
    for (u32 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(m_x) + k_dx4[i];
        const i32 ny = static_cast<i32>(m_y) + k_dy4[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h || !is_walk(m_map.get_terrain(tx, ty))) {
            continue;
        }
        const u32 d = dist_man(tx, ty, m_tgt_x, m_tgt_y);
        if (d < best) {
            best = d;
            bx = tx;
            by = ty;
        }
    }
    if (bx != m_x || by != m_y) {
        if (best < cur_d) {
            m_x = bx;
            m_y = by;
            m_has_skip = false;
            return true;
        }
    }
    u16 sx = m_x;
    u16 sy = m_y;
    if (bfs_step(m_map, w, h, tile_n, m_x, m_y, m_tgt_x, m_tgt_y, sx, sy)) {
        if (sx != m_x || sy != m_y) {
            m_x = sx;
            m_y = sy;
            m_has_skip = false;
            return true;
        }
    }
    path_fail("step_to_tgt fail", m_x, m_y, m_tgt_x, m_tgt_y);
    m_skip_x = m_tgt_x;
    m_skip_y = m_tgt_y;
    m_has_skip = true;
    return false;
}

void ExploreDistantMk2::move (u16 moves) {
    if (m_done) {
        return;
    }
    reveal_around();
    for (u16 m = 0; m < moves; ++m) {
        if (!m_has_tgt || (m_x == m_tgt_x && m_y == m_tgt_y)) {
            if (!pick_frontier()) {
                m_done = true;
                break;
            }
        }
        if (!step_to_tgt()) {
            m_has_tgt = false;
            continue;
        }
        reveal_around();
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
