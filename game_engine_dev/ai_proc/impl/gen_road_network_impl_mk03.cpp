//================================================================================================================================
//=> - Includes (mk03: CityConnector-style city spines + Prim tree) -
//================================================================================================================================

#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Local (mk03) -
//================================================================================================================================

static const u32 k_spine_max = 128u;
static const u8 k_win_r = 20u;
static const u16 k_win = static_cast<u16>(k_win_r * 2u + 1u);
static const u16 k_win_n = static_cast<u16>(k_win * k_win);
static const u32 k_term_max = 8192u;
static const u16 k_link_r = 25u;
static const u16 k_link_win = static_cast<u16>(k_link_r * 2u + 1u);
static const u32 k_link_win_n = static_cast<u32>(k_link_win) * static_cast<u32>(k_link_win);
static const i32 k_dx8[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy8[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

struct GrnTerm {
    u16 m_x;
    u16 m_y;
};

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool tile_block (const GameArraySimple& map, u16 x, u16 y) {
    if (x >= map.width() || y >= map.height()) {
        return true;
    }
    const u8 terr = map.get_terrain(x, y);
    if (overlay_is_water_terr(terr)) {
        return true;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return true;
    }
    return false;
}

static bool step_linear (u16 x0, u16 y0, u16 x1, u16 y1, u16* ox, u16* oy) {
    if (x0 == x1 && y0 == y1) {
        return false;
    }
    const i32 ax = static_cast<i32>(x1) - static_cast<i32>(x0);
    const i32 ay = static_cast<i32>(y1) - static_cast<i32>(y0);
    const i32 adx = ax < 0 ? -ax : ax;
    const i32 ady = ay < 0 ? -ay : ay;
    const i32 sx = sgn(ax);
    const i32 sy = sgn(ay);
    i32 nx = static_cast<i32>(x0);
    i32 ny = static_cast<i32>(y0);
    if (adx > ady) {
        if (ady == 0 || (ady + ady) <= adx) {
            nx += sx;
        } else {
            nx += sx;
            ny += sy;
        }
    } else if (ady > adx) {
        if (adx == 0 || (adx + adx) <= ady) {
            ny += sy;
        } else {
            nx += sx;
            ny += sy;
        }
    } else {
        nx += sx;
        ny += sy;
    }
    if (nx < 0 || ny < 0) {
        return false;
    }
    *ox = static_cast<u16>(nx);
    *oy = static_cast<u16>(ny);
    return true;
}

static bool line_clear (const GameArraySimple& map, u16 x0, u16 y0, u16 x1, u16 y1) {
    u16 x = x0;
    u16 y = y0;
    while (x != x1 || y != y1) {
        u16 nx = 0;
        u16 ny = 0;
        if (!step_linear(x, y, x1, y1, &nx, &ny)) {
            return false;
        }
        if (tile_block(map, nx, ny)) {
            return false;
        }
        x = nx;
        y = ny;
    }
    return true;
}

static bool step_flood (
    const GameArraySimple& map,
    u16 wx,
    u16 wy,
    u16 tx,
    u16 ty,
    u16* ox,
    u16* oy)
{
    const i32 cx = (static_cast<i32>(wx) + static_cast<i32>(tx)) / 2;
    const i32 cy = (static_cast<i32>(wy) + static_cast<i32>(ty)) / 2;
    const i32 ox0 = cx - static_cast<i32>(k_win_r);
    const i32 oy0 = cy - static_cast<i32>(k_win_r);
    u16 wd[k_win_n];
    u8 qx[k_win_n];
    u8 qy[k_win_n];
    for (u16 i = 0; i < k_win_n; ++i) {
        wd[i] = U16_KEY_NULL;
    }
    const i32 tlx = static_cast<i32>(tx) - ox0;
    const i32 tly = static_cast<i32>(ty) - oy0;
    if (tlx < 0 || tly < 0 || tlx >= static_cast<i32>(k_win) || tly >= static_cast<i32>(k_win)) {
        return false;
    }
    const i32 wlx = static_cast<i32>(wx) - ox0;
    const i32 wly = static_cast<i32>(wy) - oy0;
    if (wlx < 0 || wly < 0 || wlx >= static_cast<i32>(k_win) || wly >= static_cast<i32>(k_win)) {
        return false;
    }
    const u16 mw = map.width();
    const u16 mh = map.height();
    const u32 tidx = static_cast<u32>(tly) * k_win + static_cast<u32>(tlx);
    wd[tidx] = 0;
    qx[0] = static_cast<u8>(tlx);
    qy[0] = static_cast<u8>(tly);
    u16 qh = 0;
    u16 qt = 1;
    while (qh < qt) {
        const u8 lx = qx[qh];
        const u8 ly = qy[qh];
        ++qh;
        const u16 cd = wd[static_cast<u32>(ly) * k_win + static_cast<u32>(lx)];
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nlx = static_cast<i32>(lx) + k_dx8[d];
            const i32 nly = static_cast<i32>(ly) + k_dy8[d];
            if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(k_win) || nly >= static_cast<i32>(k_win)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(nly) * k_win + static_cast<u32>(nlx);
            if (wd[nidx] != U16_KEY_NULL) {
                continue;
            }
            const i32 gx = ox0 + nlx;
            const i32 gy = oy0 + nly;
            if (gx < 0 || gy < 0 || gx >= static_cast<i32>(mw) || gy >= static_cast<i32>(mh)) {
                continue;
            }
            const u16 ux = static_cast<u16>(gx);
            const u16 uy = static_cast<u16>(gy);
            if (!(ux == wx && uy == wy) && !(ux == tx && uy == ty) && tile_block(map, ux, uy)) {
                continue;
            }
            wd[nidx] = static_cast<u16>(cd + 1u);
            if (qt >= k_win_n) {
                break;
            }
            qx[qt] = static_cast<u8>(nlx);
            qy[qt] = static_cast<u8>(nly);
            ++qt;
        }
    }
    if (wd[static_cast<u32>(wly) * k_win + static_cast<u32>(wlx)] == U16_KEY_NULL) {
        return false;
    }
    u16 best_d = U16_KEY_NULL;
    u16 bx = 0;
    u16 by = 0;
    bool hit = false;
    for (u8 d = 0; d < 8u; ++d) {
        const i32 nx = static_cast<i32>(wx) + k_dx8[d];
        const i32 ny = static_cast<i32>(wy) + k_dy8[d];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(mw) || ny >= static_cast<i32>(mh)) {
            continue;
        }
        const i32 nlx = nx - ox0;
        const i32 nly = ny - oy0;
        if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(k_win) || nly >= static_cast<i32>(k_win)) {
            continue;
        }
        const u16 nd = wd[static_cast<u32>(nly) * k_win + static_cast<u32>(nlx)];
        if (nd == U16_KEY_NULL || nd >= best_d) {
            continue;
        }
        best_d = nd;
        bx = static_cast<u16>(nx);
        by = static_cast<u16>(ny);
        hit = true;
    }
    if (!hit) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

static bool pick_step (
    const GameArraySimple& map,
    u16 wx,
    u16 wy,
    u16 tx,
    u16 ty,
    u16* ox,
    u16* oy)
{
    if (line_clear(map, wx, wy, tx, ty)) {
        return step_linear(wx, wy, tx, ty, ox, oy);
    }
    return step_flood(map, wx, wy, tx, ty, ox, oy);
}

static u16 next_seg (u16* seg_n) {
    if (*seg_n == 0xffffu) {
        *seg_n = 1u;
    } else {
        *seg_n = static_cast<u16>(*seg_n + 1u);
    }
    return *seg_n;
}

static void stamp_tile (
    GameArraySimple& map,
    Whiteboard_2B& road,
    u32* road_n,
    u16 seg,
    u16 x,
    u16 y)
{
    if (road.rd(x, y) != 0u) {
        return;
    }
    road.wr(x, y, seg);
    *road_n = *road_n + 1u;
    if (map.get_road_typ(x, y) == ROAD_NONE) {
        map.set_road_typ(x, y, ROAD_VIRTUAL);
    }
}

static void stamp_spine (
    GameArraySimple& map,
    Whiteboard_2B& road,
    u32* road_n,
    u16* seg_n,
    u16 x0,
    u16 y0,
    u16 x1,
    u16 y1)
{
    const u16 seg = next_seg(seg_n);
    u16 x = x0;
    u16 y = y0;
    u32 n = 0;
    for (;;) {
        stamp_tile(map, road, road_n, seg, x, y);
        ++n;
        if (x == x1 && y == y1) {
            break;
        }
        if (n >= k_spine_max) {
            break;
        }
        u16 nx = 0;
        u16 ny = 0;
        if (!pick_step(map, x, y, x1, y1, &nx, &ny)) {
            break;
        }
        x = nx;
        y = ny;
    }
}

static bool term_has (const GrnTerm* terms, u32 n, u16 x, u16 y) {
    for (u32 i = 0; i < n; ++i) {
        if (terms[i].m_x == x && terms[i].m_y == y) {
            return true;
        }
    }
    return false;
}

static void term_add (const GameArraySimple& map, GrnTerm* terms, u32* n, u32 cap, u16 x, u16 y) {
    if (*n >= cap || tile_block(map, x, y) || term_has(terms, *n, x, y)) {
        return;
    }
    terms[*n].m_x = x;
    terms[*n].m_y = y;
    *n = *n + 1u;
}

static u32 collect_terms (
    const GameArraySimple& map,
    const SpgCoordPair* starts,
    u32 start_n,
    GrnTerm* terms,
    u32 cap)
{
    u32 n = 0;
    for (u32 i = 0; i < start_n; ++i) {
        term_add(map, terms, &n, cap, starts[i].x, starts[i].y);
    }
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_ai_ov_intent(x, y) == AI_TILE_OV_INTENT_CITY) {
                term_add(map, terms, &n, cap, x, y);
            }
        }
    }
    return n;
}

static int quad_of_xy (u16 sx, u16 sy, u16 dx, u16 dy) {
    const i32 qx = static_cast<i32>(dx) - static_cast<i32>(sx);
    const i32 qy = static_cast<i32>(dy) - static_cast<i32>(sy);
    if (qx == 0 && qy == 0) {
        return -1;
    }
    if (qx >= 0 && qy < 0) {
        return 0;
    }
    if (qx < 0 && qy <= 0) {
        return 1;
    }
    if (qx <= 0 && qy > 0) {
        return 3;
    }
    return 2;
}

static void flood_quads (
    const GameArraySimple& map,
    const Whiteboard_2B& city_at,
    const GrnTerm* terms,
    u16 src,
    u16 out_best[4])
{
    out_best[0] = U16_KEY_NULL;
    out_best[1] = U16_KEY_NULL;
    out_best[2] = U16_KEY_NULL;
    out_best[3] = U16_KEY_NULL;
    u16 best_d[4] = {U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL};
    const u16 sx = terms[src].m_x;
    const u16 sy = terms[src].m_y;
    const i32 half = static_cast<i32>(k_link_r);
    const i32 ox = static_cast<i32>(sx) - half;
    const i32 oy = static_cast<i32>(sy) - half;
    u16 wd[k_link_win_n];
    u8 qx[k_link_win_n];
    u8 qy[k_link_win_n];
    for (u32 i = 0; i < k_link_win_n; ++i) {
        wd[i] = U16_KEY_NULL;
    }
    const u32 sidx = static_cast<u32>(half) * k_link_win + static_cast<u32>(half);
    wd[sidx] = 0;
    u32 qn = 0;
    qx[qn] = static_cast<u8>(half);
    qy[qn] = static_cast<u8>(half);
    ++qn;
    const u16 mw = map.width();
    const u16 mh = map.height();
    for (u32 qh = 0; qh < qn; ++qh) {
        const u8 lx = qx[qh];
        const u8 ly = qy[qh];
        const u16 d0 = wd[static_cast<u32>(ly) * k_link_win + static_cast<u32>(lx)];
        const i32 wx = ox + static_cast<i32>(lx);
        const i32 wy = oy + static_cast<i32>(ly);
        if (wx >= 0 && wy >= 0 && wx < static_cast<i32>(mw) && wy < static_cast<i32>(mh)) {
            const u16 ux = static_cast<u16>(wx);
            const u16 uy = static_cast<u16>(wy);
            const u16 j = city_at.rd(ux, uy);
            if (j != U16_KEY_NULL && j != src && d0 > 0u && d0 <= k_link_r) {
                const int q = quad_of_xy(sx, sy, ux, uy);
                if (q >= 0) {
                    if (out_best[q] == U16_KEY_NULL || d0 < best_d[q]) {
                        out_best[q] = j;
                        best_d[q] = d0;
                    }
                }
            }
        }
        if (out_best[0] != U16_KEY_NULL && out_best[1] != U16_KEY_NULL
            && out_best[2] != U16_KEY_NULL && out_best[3] != U16_KEY_NULL) {
            break;
        }
        if (d0 >= k_link_r) {
            continue;
        }
        for (i32 dir = 0; dir < 4; ++dir) {
            const i32 nlx = static_cast<i32>(lx) + k_dx4[dir];
            const i32 nly = static_cast<i32>(ly) + k_dy4[dir];
            if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(k_link_win)
                || nly >= static_cast<i32>(k_link_win)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(nly) * k_link_win + static_cast<u32>(nlx);
            if (wd[nidx] != U16_KEY_NULL) {
                continue;
            }
            const i32 nx = ox + nlx;
            const i32 ny = oy + nly;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(mw) || ny >= static_cast<i32>(mh)) {
                continue;
            }
            if (tile_block(map, static_cast<u16>(nx), static_cast<u16>(ny))) {
                continue;
            }
            wd[nidx] = static_cast<u16>(d0 + 1u);
            qx[qn] = static_cast<u8>(nlx);
            qy[qn] = static_cast<u8>(nly);
            ++qn;
        }
    }
}

static void connect_quad4 (
    GameArraySimple& map,
    Whiteboard_2B& road,
    Whiteboard_2B& city_at,
    u32* road_n,
    u16* seg_n,
    const GrnTerm* terms,
    u32 tn)
{
    if (tn < 2u) {
        if (tn == 1u) {
            stamp_tile(map, road, road_n, next_seg(seg_n), terms[0].m_x, terms[0].m_y);
        }
        return;
    }
    const u32 mtn = map.tile_n();
    for (u32 i = 0; i < mtn; ++i) {
        city_at.wr_i(i, U16_KEY_NULL);
    }
    for (u32 i = 0; i < tn; ++i) {
        city_at.wr(terms[i].m_x, terms[i].m_y, static_cast<u16>(i));
    }
    u16 links[k_term_max][4];
    for (u32 i = 0; i < tn; ++i) {
        flood_quads(map, city_at, terms, static_cast<u16>(i), links[i]);
    }
    for (u32 i = 0; i < tn; ++i) {
        for (u8 q = 0; q < 4u; ++q) {
            const u16 j = links[i][q];
            if (j == U16_KEY_NULL || j <= i) {
                continue;
            }
            stamp_spine(
                map, road, road_n, seg_n, terms[i].m_x, terms[i].m_y, terms[j].m_x, terms[j].m_y);
        }
    }
}

//================================================================================================================================
//=> - GenRoadNetwork (mk03) -
//================================================================================================================================

bool GenRoadNetwork::build (const SpgCoordPair* starts, u32 start_n) {
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenRoadNetwork build not begun");
    GameArraySimple& map = *m_map;
    const u32 tn = map.tile_n();
    std::memset(m_road.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    std::memset(m_foll.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    m_road_n = 0;
    m_term_n = 0;
    m_seg_n = 0;
    GrnTerm terms[k_term_max];
    const u32 n = collect_terms(map, starts, start_n, terms, k_term_max);
    m_term_n = n;
    connect_quad4(map, m_road, m_foll, &m_road_n, &m_seg_n, terms, n);
    return true;
}

//================================================================================================================================
//=> - End of mk03 -
//================================================================================================================================
