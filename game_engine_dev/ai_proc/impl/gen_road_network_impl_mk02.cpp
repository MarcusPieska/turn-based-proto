//================================================================================================================================
//=> - Includes (mk02: tandem road/river follow, segment 2B road+foll) -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "assert_log.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Local (mk02) -
//================================================================================================================================

static const i8 k_dx4[4] = {0, 1, 0, -1};
static const i8 k_dy4[4] = {-1, 0, 1, 0};
static const i8 k_dx8[8] = {1, 1, 0, -1, -1, -1, 0, 1};
static const i8 k_dy8[8] = {0, 1, 1, 1, 0, -1, -1, -1};

static u32 g_rng = 1u;

struct PlaceQ {
    u16 m_x0;
    u16 m_y0;
    u16 m_x1;
    u16 m_y1;
    u8 m_n;
};

#ifndef GEN_ROAD_DBG
#define GEN_ROAD_DBG 0
#endif

static u32 tix (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static u32 rnd () {
    g_rng = g_rng * 1664525u + 1013904223u;
    return g_rng;
}

static bool in_b (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && x < static_cast<i32>(w) && y < static_cast<i32>(h);
}

static bool adj8 (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(bx) - static_cast<i32>(ax);
    const i32 dy = static_cast<i32>(by) - static_cast<i32>(ay);
    const i32 adx = dx < 0 ? -dx : dx;
    const i32 ady = dy < 0 ? -dy : dy;
    return adx <= 1 && ady <= 1 && (adx + ady) > 0;
}

#if GEN_ROAD_DBG
static void grn_dbg_follow_adj (u16 lx, u16 ly, u16 x, u16 y, u8 qn) {
    if (qn == 0u) {
        return;
    }
    if (adj8(lx, ly, x, y)) {
        return;
    }
    std::fprintf(stderr,
        "GEN_ROAD_DBG: follow-up (%u,%u) not 8-adj to last (%u,%u) qn=%u\n",
        (unsigned)x, (unsigned)y, (unsigned)lx, (unsigned)ly, (unsigned)qn);
    std::abort();
}
#define GRN_DBG_FOLLOW_ADJ(lx, ly, x, y, qn) grn_dbg_follow_adj((lx), (ly), (x), (y), (qn))
#else
#define GRN_DBG_FOLLOW_ADJ(lx, ly, x, y, qn) ((void)0)
#endif

static bool is_riv (const GameArraySimple& map, u16 x, u16 y) {
    return map.get_river(x, y) != 0u;
}

static bool is_inland_wtr (const GameArraySimple& map, u16 x, u16 y) {
    const u8 terr = map.get_terrain(x, y);
    return terr == TERR_INLAND_LAKE[0] || terr == TERR_INLAND_SEA[0];
}

static bool is_spine (const GameArraySimple& map, u16 x, u16 y) {
    return is_riv(map, x, y) || is_inland_wtr(map, x, y);
}

static bool bank_land_ok (const GameArraySimple& map, u16 x, u16 y) {
    if (is_riv(map, x, y)) {
        return false;
    }
    const u8 terr = map.get_terrain(x, y);
    if (overlay_is_water_terr(terr) || terr == TERR_NONE[0]) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0]) {
        const u8 iv = map.get_ai_ov_intent(x, y);
        return iv == AI_TILE_OV_INTENT_MTN_PASS || iv == AI_TILE_OV_INTENT_FORT;
    }
    return true;
}

static bool step8_ok (const GameArraySimple& map, u16 fx, u16 fy, u16 tx, u16 ty) {
    const i32 dx = static_cast<i32>(tx) - static_cast<i32>(fx);
    const i32 dy = static_cast<i32>(ty) - static_cast<i32>(fy);
    const i32 adx = dx < 0 ? -dx : dx;
    const i32 ady = dy < 0 ? -dy : dy;
    if (adx > 1 || ady > 1 || (adx == 0 && ady == 0)) {
        return false;
    }
    if (adx + ady == 1) {
        return true;
    }
    const u16 ox = static_cast<u16>(static_cast<i32>(fx) + dx);
    const u16 oy = fy;
    const u16 px = fx;
    const u16 py = static_cast<u16>(static_cast<i32>(fy) + dy);
    if (is_riv(map, ox, oy) || is_riv(map, px, py)) {
        return false;
    }
    if (is_inland_wtr(map, ox, oy) || is_inland_wtr(map, px, py)) {
        return false;
    }
    return true;
}

static bool spine_on_seg_front (
    const GameArraySimple& map,
    const Whiteboard_2B& foll,
    u16 seg,
    u16 sx,
    u16 sy)
{
    const u16 w = map.width();
    const u16 h = map.height();
    for (u8 d = 0; d < 4u; ++d) {
        const i32 nx = static_cast<i32>(sx) + static_cast<i32>(k_dx4[d]);
        const i32 ny = static_cast<i32>(sy) + static_cast<i32>(k_dy4[d]);
        if (!in_b(w, h, nx, ny)) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (is_spine(map, ux, uy) && foll.rd(ux, uy) == seg) {
            return true;
        }
    }
    return false;
}

static bool touches_seg_front (
    const GameArraySimple& map,
    const Whiteboard_1B& comp,
    const Whiteboard_2B& foll,
    u16 seg,
    u16 x,
    u16 y,
    u8* o_n,
    u8* o_front)
{
    const u16 w = map.width();
    const u16 h = map.height();
    u8 n = 0;
    u8 front = 0;
    for (u8 k = 0; k < 8u; ++k) {
        const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx8[k]);
        const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy8[k]);
        if (!in_b(w, h, nx, ny)) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_spine(map, ux, uy) || comp.rd(ux, uy) == 0u || foll.rd(ux, uy) != 0u) {
            continue;
        }
        ++n;
        if (spine_on_seg_front(map, foll, seg, ux, uy)) {
            ++front;
        }
    }
    if (o_n != nullptr) {
        *o_n = n;
    }
    if (o_front != nullptr) {
        *o_front = front;
    }
    return front > 0u;
}

static bool cand_ok (
    const GameArraySimple& map,
    const Whiteboard_1B& comp,
    const Whiteboard_2B& road,
    const Whiteboard_1B& used,
    const Whiteboard_2B& foll,
    u16 seg,
    u16 fx,
    u16 fy,
    u16 tx,
    u16 ty,
    u8* o_n,
    u8* o_front)
{
    if (road.rd(tx, ty) != 0u || used.rd(tx, ty) != 0u) {
        return false;
    }
    if (!bank_land_ok(map, tx, ty)) {
        return false;
    }
    if (!step8_ok(map, fx, fy, tx, ty)) {
        return false;
    }
    return touches_seg_front(map, comp, foll, seg, tx, ty, o_n, o_front);
}

static u8 mark_riv_at_tip (
    const GameArraySimple& map,
    const Whiteboard_1B& comp,
    Whiteboard_2B& foll,
    u16 seg,
    u16 x,
    u16 y)
{
    const u16 w = map.width();
    const u16 h = map.height();
    u8 n = 0;
    for (u8 k = 0; k < 8u; ++k) {
        const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx8[k]);
        const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy8[k]);
        if (!in_b(w, h, nx, ny)) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_spine(map, ux, uy) || comp.rd(ux, uy) == 0u) {
            continue;
        }
        if (foll.rd(ux, uy) != 0u) {
            continue;
        }
        foll.wr(ux, uy, seg);
        ++n;
    }
    return n;
}

static void unstamp_road (GameArraySimple& map, Whiteboard_2B& road, u32* road_n, u16 x, u16 y) {
    if (road.rd(x, y) == 0u) {
        return;
    }
    road.wr(x, y, 0u);
    if (*road_n > 0u) {
        *road_n = *road_n - 1u;
    }
    if (road_is_virtual(map.get_road_typ(x, y))) {
        map.set_road_typ(x, y, ROAD_NONE);
    }
}

static bool place_mark (
    GameArraySimple& map,
    Whiteboard_2B& road,
    Whiteboard_1B& used,
    u32* road_n,
    PlaceQ* q,
    u16 seg,
    u16 x,
    u16 y)
{
    if (road.rd(x, y) != 0u || used.rd(x, y) != 0u) {
        return false;
    }
    if (!bank_land_ok(map, x, y)) {
        return false;
    }
    if (q->m_n == 2u && adj8(x, y, q->m_x0, q->m_y0) && adj8(x, y, q->m_x1, q->m_y1)) {
        unstamp_road(map, road, road_n, q->m_x1, q->m_y1);
        q->m_n = 1u;
    }
    if (q->m_n > 0u) {
        const u16 lx = (q->m_n == 2u) ? q->m_x1 : q->m_x0;
        const u16 ly = (q->m_n == 2u) ? q->m_y1 : q->m_y0;
        GRN_DBG_FOLLOW_ADJ(lx, ly, x, y, q->m_n);
    }
    used.wr(x, y, 1u);
    road.wr(x, y, seg);
    *road_n = *road_n + 1u;
    if (map.get_road_typ(x, y) == ROAD_NONE) {
        map.set_road_typ(x, y, ROAD_VIRTUAL);
    }
    if (q->m_n == 2u) {
        q->m_x0 = q->m_x1;
        q->m_y0 = q->m_y1;
        q->m_n = 1u;
    }
    if (q->m_n == 0u) {
        q->m_x0 = x;
        q->m_y0 = y;
        q->m_n = 1u;
    } else {
        q->m_x1 = x;
        q->m_y1 = y;
        q->m_n = 2u;
    }
    return true;
}

static u16 next_seg (u16* seg_n) {
    if (*seg_n == 0xffffu) {
        *seg_n = 1u;
    } else {
        *seg_n = static_cast<u16>(*seg_n + 1u);
    }
    return *seg_n;
}

static u8 spine_deg4 (const GameArraySimple& map, u16 w, u16 h, u16 x, u16 y) {
    u8 n = 0;
    for (u8 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx4[k]);
        const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy4[k]);
        if (!in_b(w, h, nx, ny)) {
            continue;
        }
        if (is_spine(map, static_cast<u16>(nx), static_cast<u16>(ny))) {
            ++n;
        }
    }
    return n;
}

static bool flood_comp (
    const GameArraySimple& map,
    Whiteboard_1B& seen,
    Whiteboard_1B& comp,
    Whiteboard_4B& que,
    u16 sx,
    u16 sy,
    u32* o_n,
    u32* o_min_i,
    u16* o_ex,
    u16* o_ey)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    std::memset(comp.raw(), 0, static_cast<size_t>(tn));
    u32 qh = 0;
    u32 qt = 0;
    const u32 si = tix(w, sx, sy);
    que.wr_i(qt++, si);
    seen.wr(sx, sy, 1u);
    comp.wr(sx, sy, 1u);
    u32 min_i = si;
    u16 ex = sx;
    u16 ey = sy;
    u8 best_deg = spine_deg4(map, w, h, sx, sy);
    while (qh < qt) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (i < min_i) {
            min_i = i;
        }
        const u8 deg = spine_deg4(map, w, h, x, y);
        if (deg < best_deg || (deg == best_deg && i < tix(w, ex, ey))) {
            best_deg = deg;
            ex = x;
            ey = y;
        }
        for (u8 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx4[k]);
            const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy4[k]);
            if (!in_b(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_spine(map, ux, uy) || seen.rd(ux, uy) != 0u) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            comp.wr(ux, uy, 1u);
            que.wr_i(qt++, tix(w, ux, uy));
        }
    }
    if (o_n != nullptr) {
        *o_n = qt;
    }
    if (o_min_i != nullptr) {
        *o_min_i = min_i;
    }
    if (o_ex != nullptr) {
        *o_ex = ex;
    }
    if (o_ey != nullptr) {
        *o_ey = ey;
    }
    return qt > 0u;
}

static bool road_phase (
    GameArraySimple& map,
    const Whiteboard_1B& comp,
    Whiteboard_2B& road,
    Whiteboard_1B& used,
    const Whiteboard_2B& foll,
    u32* road_n,
    u16* cx,
    u16* cy,
    u16 seg,
    PlaceQ* q)
{
    const u16 w = map.width();
    const u16 h = map.height();
    bool any = false;
    for (;;) {
        u16 cxs[8];
        u16 cys[8];
        u8 sc[8];
        u8 n = 0;
        for (u8 k = 0; k < 8u; ++k) {
            const i32 nx = static_cast<i32>(*cx) + static_cast<i32>(k_dx8[k]);
            const i32 ny = static_cast<i32>(*cy) + static_cast<i32>(k_dy8[k]);
            if (!in_b(w, h, nx, ny)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            u8 hit = 0;
            u8 front = 0;
            if (!cand_ok(map, comp, road, used, foll, seg, *cx, *cy, ux, uy, &hit, &front)) {
                continue;
            }
            cxs[n] = ux;
            cys[n] = uy;
            const u8 diag = (k_dx8[k] != 0 && k_dy8[k] != 0) ? 1u : 0u;
            sc[n] = static_cast<u8>(front * 16u + hit * 2u + diag);
            ++n;
        }
        if (n == 0u) {
            break;
        }
        u8 best = 0;
        for (u8 i = 1; i < n; ++i) {
            if (sc[i] > sc[best]
                || (sc[i] == sc[best]
                    && tix(w, cxs[i], cys[i]) < tix(w, cxs[best], cys[best]))) {
                best = i;
            }
        }
        if (!place_mark(map, road, used, road_n, q, seg, cxs[best], cys[best])) {
            break;
        }
        *cx = cxs[best];
        *cy = cys[best];
        any = true;
    }
    return any;
}

static void walk_seg (
    GameArraySimple& map,
    const Whiteboard_1B& comp,
    Whiteboard_2B& road,
    Whiteboard_1B& used,
    Whiteboard_2B& foll,
    u32* road_n,
    u16 sx,
    u16 sy,
    u16 seg,
    PlaceQ q)
{
    u16 cx = sx;
    u16 cy = sy;
    mark_riv_at_tip(map, comp, foll, seg, cx, cy);
    for (;;) {
        const bool did_road = road_phase(map, comp, road, used, foll, road_n, &cx, &cy, seg, &q);
        const u8 marked = mark_riv_at_tip(map, comp, foll, seg, cx, cy);
        if (!did_road && marked == 0u) {
            break;
        }
    }
}

static bool seed_at_spine (
    GameArraySimple& map,
    const Whiteboard_1B& comp,
    Whiteboard_2B& road,
    Whiteboard_1B& used,
    Whiteboard_2B& foll,
    u32* road_n,
    u16* seg_n,
    u16 sx,
    u16 sy)
{
    const u16 w = map.width();
    const u16 h = map.height();
    if (foll.rd(sx, sy) != 0u) {
        return false;
    }
    u16 cxs[8];
    u16 cys[8];
    u8 n = 0;
    for (u8 k = 0; k < 8u; ++k) {
        const i32 nx = static_cast<i32>(sx) + static_cast<i32>(k_dx8[k]);
        const i32 ny = static_cast<i32>(sy) + static_cast<i32>(k_dy8[k]);
        if (!in_b(w, h, nx, ny)) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (road.rd(ux, uy) != 0u || used.rd(ux, uy) != 0u || !bank_land_ok(map, ux, uy)) {
            continue;
        }
        cxs[n] = ux;
        cys[n] = uy;
        ++n;
    }
    if (n == 0u) {
        return false;
    }
    const u8 pick = static_cast<u8>(rnd() % n);
    const u16 bx = cxs[pick];
    const u16 by = cys[pick];
    const u16 seg = next_seg(seg_n);
    PlaceQ q;
    q.m_n = 0u;
    if (!place_mark(map, road, used, road_n, &q, seg, bx, by)) {
        return false;
    }
    if (mark_riv_at_tip(map, comp, foll, seg, bx, by) == 0u) {
        foll.wr(sx, sy, seg);
    }
    walk_seg(map, comp, road, used, foll, road_n, bx, by, seg, q);
    PlaceQ qr;
    qr.m_x0 = bx;
    qr.m_y0 = by;
    qr.m_n = 1u;
    walk_seg(map, comp, road, used, foll, road_n, bx, by, seg, qr);
    return true;
}

static void walk_comp (
    GameArraySimple& map,
    const Whiteboard_1B& comp,
    const Whiteboard_4B& mem,
    u32 cn,
    Whiteboard_2B& road,
    Whiteboard_1B& used,
    Whiteboard_2B& foll,
    u32* road_n,
    u16* seg_n,
    u16 ex,
    u16 ey)
{
    const u16 w = map.width();
    seed_at_spine(map, comp, road, used, foll, road_n, seg_n, ex, ey);
    for (u32 i = 0; i < cn; ++i) {
        const u32 ti = mem.rd_i(i);
        const u16 x = static_cast<u16>(ti % static_cast<u32>(w));
        const u16 y = static_cast<u16>(ti / static_cast<u32>(w));
        if (foll.rd(x, y) != 0u) {
            continue;
        }
        seed_at_spine(map, comp, road, used, foll, road_n, seg_n, x, y);
    }
}

static void roads_along_spine (
    GameArraySimple& map,
    Whiteboard_2B& road,
    Whiteboard_2B& foll,
    u32* road_n,
    u16* seg_n)
{
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 tn = map.tile_n();
    Whiteboard_1B seen("GenRoadNetwork", "seen", 0u);
    Whiteboard_1B comp("GenRoadNetwork", "comp", 0u);
    Whiteboard_1B used("GenRoadNetwork", "used", 0u);
    Whiteboard_4B que("GenRoadNetwork", "que", 0u);
    if (!seen.ok() || !comp.ok() || !used.ok() || !que.ok()) {
        return;
    }
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    std::memset(used.raw(), 0, static_cast<size_t>(tn));
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_spine(map, x, y) || seen.rd(x, y) != 0u) {
                continue;
            }
            u32 cn = 0;
            u32 min_i = 0;
            u16 ex = x;
            u16 ey = y;
            if (!flood_comp(map, seen, comp, que, x, y, &cn, &min_i, &ex, &ey)) {
                continue;
            }
            g_rng = min_i ^ 0x9e3779b9u;
            if (g_rng == 0u) {
                g_rng = 1u;
            }
            walk_comp(map, comp, que, cn, road, used, foll, road_n, seg_n, ex, ey);
        }
    }
}

//================================================================================================================================
//=> - GenRoadNetwork (mk02) -
//================================================================================================================================

bool GenRoadNetwork::build (const SpgCoordPair* starts, u32 start_n) {
    (void)starts;
    (void)start_n;
    GAME_EXPECT_RET(m_ok && m_map != nullptr, false, "GenRoadNetwork build not begun");
    GameArraySimple& map = *m_map;
    const u32 tn = map.tile_n();
    std::memset(m_road.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    std::memset(m_foll.raw(), 0, static_cast<size_t>(tn) * sizeof(u16));
    m_road_n = 0;
    m_term_n = 0;
    m_seg_n = 0;
    roads_along_spine(map, m_road, m_foll, &m_road_n, &m_seg_n);
    return true;
}

//================================================================================================================================
//=> - End of mk02 -
//================================================================================================================================
