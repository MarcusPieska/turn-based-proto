//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "explore_distant_mk3.h"
#include "ai_whiteboard.h"
#include "game_map_defs.h"
#include "generate_distance_to_coastal_point.h"
#include "generate_distance_to_ocean_coast.h"
#include "runtime_trace_dbg.h"

#include <ctime>

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_st_cap = 1000u;
static const u16 k_dep_none = 0xFFFFu;
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

static u8* build_terrain (const GameArraySimple& map, u16 w, u16 h) {
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[tile_n];
    if (terrain == nullptr) {
        return nullptr;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            terrain[tidx(w, x, y)] = map.get_terrain(x, y);
        }
    }
    return terrain;
}

static bool st_add (
    u16* stx,
    u16* sty,
    u16& st_n,
    u16 x,
    u16 y) {
    if (st_n >= k_st_cap) {
        return false;
    }
    if (st_n > 0u && ShortRangePathing::cheb(stx[st_n - 1u], sty[st_n - 1u], x, y) != 1u) {
        return false;
    }
    stx[st_n] = x;
    sty[st_n] = y;
    ++st_n;
    return true;
}

static bool walk_st (
    ShortRangePathing& path,
    u16& cx,
    u16& cy,
    u16 tx,
    u16 ty,
    u16* stx,
    u16* sty,
    u16& st_n) {
    bool moved = false;
    while ((cx != tx || cy != ty) && st_n < k_st_cap) {
        u16 nx = cx;
        u16 ny = cy;
        if (!path.one_step(cx, cy, tx, ty, nx, ny)) {
            break;
        }
        cx = nx;
        cy = ny;
        if (!st_add(stx, sty, st_n, cx, cy)) {
            break;
        }
        moved = true;
    }
    return moved;
}

static bool step_down (
    const GameArraySimple& map,
    const u16* dep,
    u16 w,
    u16 h,
    u16& x,
    u16& y) {
    const u32 ci = tidx(w, x, y);
    const u16 cur = dep[ci];
    if (cur == k_dep_none || cur == 0u) {
        return false;
    }
    u16 best = cur;
    u16 bx = x;
    u16 by = y;
    bool found = false;
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h || !is_walk(map.get_terrain(tx, ty))) {
            continue;
        }
        const u16 d = dep[tidx(w, tx, ty)];
        if (d == k_dep_none || d >= cur) {
            continue;
        }
        if (!found || d < best) {
            best = d;
            bx = tx;
            by = ty;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    x = bx;
    y = by;
    return true;
}

static bool step_ring_fwd (
    const u16* ring,
    const u16* coast_dep,
    u16 w,
    u16 h,
    i16 pdx,
    i16 pdy,
    u16& x,
    u16& y,
    i16& odx,
    i16& ody) {
    const u32 ci = tidx(w, x, y);
    const u16 cur = ring[ci];
    if (cur == k_dep_none) {
        return false;
    }
    const u16 want = static_cast<u16>(cur + 1u);
    u16 best_cd = 0xFFFFu;
    i32 best_dot = -32768;
    u16 bx = x;
    u16 by = y;
    i16 bdx = 0;
    i16 bdy = 0;
    bool found = false;
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 tx = static_cast<u16>(nx);
        const u16 ty = static_cast<u16>(ny);
        if (tx >= w || ty >= h) {
            continue;
        }
        const u32 ni = tidx(w, tx, ty);
        if (ring[ni] != want) {
            continue;
        }
        const u16 cd = coast_dep[ni];
        const i16 ndx = k_dx4[k];
        const i16 ndy = k_dy4[k];
        const i32 dot = static_cast<i32>(pdx) * static_cast<i32>(ndx)
            + static_cast<i32>(pdy) * static_cast<i32>(ndy);
        if (!found || cd < best_cd || (cd == best_cd && dot > best_dot)) {
            best_cd = cd;
            best_dot = dot;
            bx = tx;
            by = ty;
            bdx = ndx;
            bdy = ndy;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    x = bx;
    y = by;
    odx = bdx;
    ody = bdy;
    return true;
}

static bool pick_coast_tgt (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    const u16* dep,
    u16 w,
    u16 h,
    u16 cx,
    u16 cy,
    u16& ox,
    u16& oy) {
    u32 best = 0xFFFFFFFFu;
    bool found = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (exp.get(x, y) != 0u || !is_walk(map.get_terrain(x, y))) {
                continue;
            }
            if (dep[tidx(w, x, y)] != 0u) {
                continue;
            }
            const u32 d = dist_man(cx, cy, x, y);
            if (d >= best) {
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

//================================================================================================================================
//=> - ExploreDistantMk3 -
//================================================================================================================================

ExploreDistantMk3::ExploreDistantMk3 (
    const GameArraySimple& map,
    MapBitOverlay& explored,
    u16 sx,
    u16 sy,
    u16 sight,
    u8 player) :
    ExploreAi(map, explored, sx, sy, sight, player),
    m_wpx(nullptr),
    m_wpy(nullptr),
    m_wp_n(0),
    m_wp_i(0),
    m_done(false),
    m_derive_sec(0.0),
    m_path(map) {
    derive_path();
}

ExploreDistantMk3::~ExploreDistantMk3 () {
    delete[] m_wpx;
    delete[] m_wpy;
}

void ExploreDistantMk3::move (u16 moves) {
    if (m_done || m_wpx == nullptr) {
        return;
    }
    reveal_around();
    for (u16 m = 0; m < moves; ++m) {
        if (m_wp_i >= m_wp_n) {
            m_done = true;
            break;
        }
        while (m_wp_i < m_wp_n) {
            const u16 tx = m_wpx[m_wp_i];
            const u16 ty = m_wpy[m_wp_i];
            if (!m_path.near_pt(m_x, m_y, tx, ty)) {
                break;
            }
            ++m_wp_i;
        }
        if (m_wp_i >= m_wp_n) {
            m_done = true;
            break;
        }
        const u16 tx = m_wpx[m_wp_i];
        const u16 ty = m_wpy[m_wp_i];
        if (ShortRangePathing::cheb(m_x, m_y, tx, ty) == 1u
            && is_walk(m_map.get_terrain(tx, ty))) {
            m_x = tx;
            m_y = ty;
            reveal_around();
            continue;
        }
        u16 nx = m_x;
        u16 ny = m_y;
        if (m_path.one_step(m_x, m_y, tx, ty, nx, ny)) {
            m_x = nx;
            m_y = ny;
            reveal_around();
            continue;
        }
        if (!m_path.can_reach(m_x, m_y, tx, ty)) {
            ++m_wp_i;
        }
    }
}

void ExploreDistantMk3::reveal_around () {
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

u16 ExploreDistantMk3::path_n () const {
    return m_wp_n;
}

u16 ExploreDistantMk3::wp_i () const {
    return m_wp_i;
}

u16 ExploreDistantMk3::wp_x (u16 i) const {
    return (m_wpx != nullptr && i < m_wp_n) ? m_wpx[i] : 0u;
}

u16 ExploreDistantMk3::wp_y (u16 i) const {
    return (m_wpy != nullptr && i < m_wp_n) ? m_wpy[i] : 0u;
}

double ExploreDistantMk3::derive_sec () const {
    return m_derive_sec;
}

void ExploreDistantMk3::note_step (
    MapBitOverlay& exp,
    u16 x,
    u16 y,
    u16& st_n,
    u16* stx,
    u16* sty) {
    exp.set(x, y);
    st_add(stx, sty, st_n, x, y);
}

void ExploreDistantMk3::derive_path () {
    const clock_t t0 = clock();
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    AiWbSheet stx(static_cast<i32>(k_st_cap));
    AiWbSheet sty(static_cast<i32>(k_st_cap));
    if (!stx.ok() || !sty.ok()) {
        m_done = true;
        m_derive_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
        return;
    }
    u16 st_n = 0;
    u8* terrain = build_terrain(m_map, w, h);
    if (terrain == nullptr) {
        m_done = true;
        m_derive_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
        return;
    }
    u16 coast_max = 0;
    u16* dep = Generate_DistanceToOceanCoast::generate(terrain, w, h, &coast_max);
    if (dep == nullptr) {
        delete[] terrain;
        m_done = true;
        m_derive_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
        return;
    }
    u16 cx = m_x;
    u16 cy = m_y;
    MapBitOverlay plan(w, h);
    plan.set(cx, cy);
    bool hit_coast = false;
    i16 pdx = 0;
    i16 pdy = 0;
    while (st_n < k_st_cap) {
        if (!hit_coast && dep[tidx(w, cx, cy)] > 0u) {
            while (dep[tidx(w, cx, cy)] > 0u && st_n < k_st_cap) {
                if (!step_down(m_map, dep, w, h, cx, cy)) {
                    break;
                }
                note_step(plan, cx, cy, st_n, stx.get(), sty.get());
            }
            hit_coast = dep[tidx(w, cx, cy)] == 0u;
            pdx = 0;
            pdy = 0;
        }
        if (hit_coast && dep[tidx(w, cx, cy)] <= Generate_DistanceToCoastalPoint::k_coast_band) {
            u16 ring_max = 0;
            u16* ring = Generate_DistanceToCoastalPoint::generate(
                terrain, w, h, dep, cx, cy, &ring_max);
            if (ring != nullptr && ring[tidx(w, cx, cy)] != k_dep_none) {
                i16 rdx = pdx;
                i16 rdy = pdy;
                while (st_n < k_st_cap) {
                    i16 ndx = 0;
                    i16 ndy = 0;
                    if (!step_ring_fwd(ring, dep, w, h, rdx, rdy, cx, cy, ndx, ndy)) {
                        break;
                    }
                    rdx = ndx;
                    rdy = ndy;
                    note_step(plan, cx, cy, st_n, stx.get(), sty.get());
                }
                pdx = rdx;
                pdy = rdy;
                delete[] ring;
            } else {
                delete[] ring;
            }
        }
        u16 tx = cx;
        u16 ty = cy;
        if (!pick_coast_tgt(m_map, plan, dep, w, h, cx, cy, tx, ty)) {
            break;
        }
        if (!m_path.can_reach(cx, cy, tx, ty)) {
            plan.set(tx, ty);
            continue;
        }
        if (tx != cx || ty != cy) {
            walk_st(m_path, cx, cy, tx, ty, stx.get(), sty.get(), st_n);
            plan.set(cx, cy);
        }
        hit_coast = dep[tidx(w, cx, cy)] == 0u;
        pdx = 0;
        pdy = 0;
    }
    delete[] dep;
    delete[] terrain;
    m_wp_n = st_n;
    if (m_wp_n > 0u) {
        m_wpx = new u16[m_wp_n];
        m_wpy = new u16[m_wp_n];
        if (m_wpx != nullptr && m_wpy != nullptr) {
            for (u16 i = 0; i < m_wp_n; ++i) {
                m_wpx[i] = stx.rd(i);
                m_wpy[i] = sty.rd(i);
            }
        } else {
            delete[] m_wpx;
            delete[] m_wpy;
            m_wpx = nullptr;
            m_wpy = nullptr;
            m_wp_n = 0;
            m_done = true;
        }
    }
    m_derive_sec = static_cast<double>(clock() - t0) / static_cast<double>(CLOCKS_PER_SEC);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
