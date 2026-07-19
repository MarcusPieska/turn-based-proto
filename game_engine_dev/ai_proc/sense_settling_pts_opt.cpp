//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sense_settling_pts_opt.h"
#include "circular_tile_areas.h"
#include "city.h"
#include "city_array.h"
#include "city_blocking_mask.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "whiteboard_mng.h"

#include <cstring>

//================================================================================================================================
//=> - Window / queues -
//================================================================================================================================

static const u16 k_r_near = 3u;
static const u16 k_r_outer = 17u;
static const u16 k_win = static_cast<u16>(2u * k_r_outer + 1u);
static const u16 k_half = k_r_outer;
static const u16 k_out_que = 256u;
static const u16 k_riv_que = 20u;
static const u8 k_sc_grass = 2u;

//================================================================================================================================
//=> - Live / city scratch -
//================================================================================================================================

struct LivePicks {
    SmSettlerBestPts pts;
    Whiteboard_1B* excl;
};

struct QuePt {
    u16 x;
    u16 y;
};

struct BestNp {
    u16 x;
    u16 y;
    u8 score;
};

struct CircQue {
    QuePt* pts;
    u16 cap;
    u16 h;
    u16 t;
    u16 n;
};

//================================================================================================================================
//=> - Flood overlays (rolling id) -
//================================================================================================================================

static u8 g_out_vis[k_win][k_win];
static u8 g_riv_vis[k_win][k_win];
static u8 g_out_id = 0;
static u8 g_riv_id = 0;

static u8 next_id (u8* id, u8 vis[k_win][k_win]) {
    *id = static_cast<u8>(*id + 1u);
    if (*id == 0) {
        std::memset(vis, 0, static_cast<size_t>(k_win) * static_cast<size_t>(k_win));
        *id = 1u;
    }
    return *id;
}

//================================================================================================================================
//=> - Filters -
//================================================================================================================================

static bool is_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static bool own_ok (u8 own, u16 player) {
    return own == U8_KEY_NULL || static_cast<u16>(own) == player;
}

static bool clim_ok (u8 clim, u8 riv) {
    if (clim == CLIMATE_BLACK_SOIL || clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS) {
        return true;
    }
    return clim == CLIMATE_DESERT && riv != 0;
}

static bool settle_ok (const GameArraySimple& map, u16 px, u16 py, u16 player) {
    const u8 terr = map.get_terrain(px, py);
    if (terr != TERR_PLAINS[0] && terr != TERR_HILLS[0]) {
        return false;
    }
    if (!clim_ok(map.get_climate(px, py), map.get_river(px, py))) {
        return false;
    }
    if (map.get_settler_blocked(px, py) != 0) {
        return false;
    }
    return own_ok(map.get_civ_owner(px, py), player);
}

static bool adj_coastal (const GameArraySimple& map, u16 w, u16 h, u16 px, u16 py) {
    if (px > 0 && is_water(map.get_terrain(static_cast<u16>(px - 1u), py))) {
        return true;
    }
    if (static_cast<u32>(px) + 1u < static_cast<u32>(w) && is_water(map.get_terrain(static_cast<u16>(px + 1u), py))) {
        return true;
    }
    if (py > 0 && is_water(map.get_terrain(px, static_cast<u16>(py - 1u)))) {
        return true;
    }
    if (static_cast<u32>(py) + 1u < static_cast<u32>(h) && is_water(map.get_terrain(px, static_cast<u16>(py + 1u)))) {
        return true;
    }
    return false;
}

static bool is_premium (const GameArraySimple& map, u16 w, u16 h, u16 px, u16 py) {
    const u8 clim = map.get_climate(px, py);
    const u8 riv = map.get_river(px, py);
    if (clim == CLIMATE_BLACK_SOIL) {
        return true;
    }
    if (riv == 0) {
        return false;
    }
    if (clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS) {
        return true;
    }
    return adj_coastal(map, w, h, px, py);
}

//================================================================================================================================
//=> - Geometry -
//================================================================================================================================

static bool in_disk (i32 dx, i32 dy, u16 r) {
    return dx * dx + dy * dy < static_cast<i32>(r) * static_cast<i32>(r);
}

static bool past_near (i32 dx, i32 dy) {
    return dx * dx + dy * dy >= static_cast<i32>(k_r_near) * static_cast<i32>(k_r_near);
}

static bool in_win (u16 ox, u16 oy, u16 x, u16 y, i32* lx, i32* ly) {
    *lx = static_cast<i32>(x) - static_cast<i32>(ox) + static_cast<i32>(k_half);
    *ly = static_cast<i32>(y) - static_cast<i32>(oy) + static_cast<i32>(k_half);
    return *lx >= 0 && *ly >= 0 && *lx < static_cast<i32>(k_win) && *ly < static_cast<i32>(k_win);
}

//================================================================================================================================
//=> - Exclude / take / best NP -
//================================================================================================================================

static void stamp_excl (Whiteboard_1B& wb, const GameArraySimple& map, u16 cx, u16 cy) {
    const u8* m = CityBlockingMask::preview(map, cx, cy);
    const u16 side = CityBlockingMask::m_side;
    const u16 half = static_cast<u16>(side / 2u);
    const u16 bw = wb.w();
    const u16 bh = wb.h();
    for (u16 ly = 0; ly < side; ++ly) {
        for (u16 lx = 0; lx < side; ++lx) {
            if (m[static_cast<u32>(ly) * static_cast<u32>(side) + static_cast<u32>(lx)] == 0) {
                continue;
            }
            const i32 x = static_cast<i32>(cx) + static_cast<i32>(lx) - static_cast<i32>(half);
            const i32 y = static_cast<i32>(cy) + static_cast<i32>(ly) - static_cast<i32>(half);
            if (x < 0 || y < 0 || x >= static_cast<i32>(bw) || y >= static_cast<i32>(bh)) {
                continue;
            }
            wb.wr(static_cast<u32>(x), static_cast<u32>(y), 1u);
        }
    }
}

static bool take_pt (const GameArraySimple& map, LivePicks* live, u16 px, u16 py) {
    if (live->pts.n >= SMS_BEST_N) {
        return false;
    }
    if (live->excl->rd(px, py) != 0) {
        return false;
    }
    live->pts.pts[live->pts.n].x = px;
    live->pts.pts[live->pts.n].y = py;
    live->pts.n = static_cast<u16>(live->pts.n + 1u);
    stamp_excl(*live->excl, map, px, py);
    return true;
}

static u8 np_score (u8 clim) {
    if (clim == CLIMATE_PLAINS) {
        return 1u;
    }
    if (clim == CLIMATE_GRASSLAND) {
        return k_sc_grass;
    }
    return 0;
}

static void note_best (const GameArraySimple& map, LivePicks* live, u16 px, u16 py, BestNp* best) {
    if (best->score >= k_sc_grass) {
        return;
    }
    if (live->excl->rd(px, py) != 0) {
        return;
    }
    const u8 sc = np_score(map.get_climate(px, py));
    if (sc <= best->score) {
        return;
    }
    best->x = px;
    best->y = py;
    best->score = sc;
}

//================================================================================================================================
//=> - Circular queue -
//================================================================================================================================

static void cq_init (CircQue* q, QuePt* buf, u16 cap) {
    q->pts = buf;
    q->cap = cap;
    q->h = 0;
    q->t = 0;
    q->n = 0;
}

static bool cq_push (CircQue* q, u16 x, u16 y) {
    if (q->n >= q->cap) {
        return false;
    }
    q->pts[q->t].x = x;
    q->pts[q->t].y = y;
    q->t = static_cast<u16>((q->t + 1u) % q->cap);
    q->n = static_cast<u16>(q->n + 1u);
    return true;
}

static bool cq_pop (CircQue* q, u16* x, u16* y) {
    if (q->n == 0) {
        return false;
    }
    *x = q->pts[q->h].x;
    *y = q->pts[q->h].y;
    q->h = static_cast<u16>((q->h + 1u) % q->cap);
    q->n = static_cast<u16>(q->n - 1u);
    return true;
}

static bool try_enq_out (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u16 ox,
    u16 oy,
    u16 x,
    u16 y,
    u8 id,
    CircQue* q)
{
    if (x >= w || y >= h) {
        return true;
    }
    const i32 dx = static_cast<i32>(x) - static_cast<i32>(ox);
    const i32 dy = static_cast<i32>(y) - static_cast<i32>(oy);
    if (!in_disk(dx, dy, k_r_outer)) {
        return true;
    }
    i32 lx = 0;
    i32 ly = 0;
    if (!in_win(ox, oy, x, y, &lx, &ly)) {
        return true;
    }
    if (g_out_vis[ly][lx] == id) {
        return true;
    }
    if (is_water(map.get_terrain(x, y))) {
        return true;
    }
    g_out_vis[ly][lx] = id;
    return cq_push(q, x, y);
}

//================================================================================================================================
//=> - Inner river flood -
//================================================================================================================================

static bool flood_river (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u16 ox,
    u16 oy,
    u16 sx,
    u16 sy,
    u16 player,
    LivePicks* live,
    BestNp* best)
{
    QuePt buf[k_riv_que];
    CircQue q;
    const u8 id = next_id(&g_riv_id, g_riv_vis);
    cq_init(&q, buf, k_riv_que);
    i32 slx = 0;
    i32 sly = 0;
    if (!in_win(ox, oy, sx, sy, &slx, &sly)) {
        return false;
    }
    g_riv_vis[sly][slx] = id;
    cq_push(&q, sx, sy);
    bool took = false;
    static const i16 k_dx[4] = {-1, 1, 0, 0};
    static const i16 k_dy[4] = {0, 0, -1, 1};
    u16 cx = 0;
    u16 cy = 0;
    while (cq_pop(&q, &cx, &cy)) {
        const i32 dx = static_cast<i32>(cx) - static_cast<i32>(ox);
        const i32 dy = static_cast<i32>(cy) - static_cast<i32>(oy);
        if (past_near(dx, dy) && in_disk(dx, dy, k_r_outer) && settle_ok(map, cx, cy, player)) {
            if (is_premium(map, w, h, cx, cy)) {
                if (take_pt(map, live, cx, cy)) {
                    took = true;
                    if (live->pts.n >= SMS_BEST_N) {
                        return true;
                    }
                }
            } else {
                note_best(map, live, cx, cy, best);
            }
        }
        for (u16 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(cx) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(cy) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            const i32 rdx = nx - static_cast<i32>(ox);
            const i32 rdy = ny - static_cast<i32>(oy);
            if (!in_disk(rdx, rdy, k_r_outer)) {
                continue;
            }
            i32 lx = 0;
            i32 ly = 0;
            if (!in_win(ox, oy, ux, uy, &lx, &ly)) {
                continue;
            }
            if (g_riv_vis[ly][lx] == id) {
                continue;
            }
            if (map.get_river(ux, uy) == 0) {
                continue;
            }
            g_riv_vis[ly][lx] = id;
            if (!cq_push(&q, ux, uy)) {
                break;
            }
        }
    }
    return took;
}

//================================================================================================================================
//=> - Outer land flood -
//================================================================================================================================

static u32 flood_city (
    const GameArraySimple& map,
    u16 w,
    u16 h,
    u16 sx,
    u16 sy,
    u16 player,
    LivePicks* live,
    BestNp* best)
{
    QuePt buf[k_out_que];
    CircQue q;
    const u8 id = next_id(&g_out_id, g_out_vis);
    cq_init(&q, buf, k_out_que);
    bool riv_done = false;
    u32 settled = 0;
    if (map.get_river(sx, sy) != 0) {
        riv_done = true;
        if (flood_river(map, w, h, sx, sy, sx, sy, player, live, best)) {
            return 1u;
        }
    }
    {
        const CircArea near = CircularTileAreas::get(k_r_near);
        if (near.m_lim != 0 && near.m_brd != nullptr) {
            for (u16 i = 0; i < near.m_lim && !riv_done; ++i) {
                const i32 px = static_cast<i32>(sx) + static_cast<i32>(near.m_brd[i][0]);
                const i32 py = static_cast<i32>(sy) + static_cast<i32>(near.m_brd[i][1]);
                if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(px);
                const u16 uy = static_cast<u16>(py);
                if (map.get_river(ux, uy) == 0) {
                    continue;
                }
                riv_done = true;
                if (flood_river(map, w, h, sx, sy, ux, uy, player, live, best)) {
                    return 1u;
                }
            }
        }
    }
    {
        const CircArea a0 = CircularTileAreas::get(k_r_near);
        const CircArea a1 = CircularTileAreas::get(static_cast<u16>(k_r_near + 1u));
        if (a1.m_lim == 0 || a1.m_brd == nullptr || a0.m_lim > a1.m_lim) {
            return 0;
        }
        for (u16 i = a0.m_lim; i < a1.m_lim; ++i) {
            const i32 px = static_cast<i32>(sx) + static_cast<i32>(a1.m_brd[i][0]);
            const i32 py = static_cast<i32>(sy) + static_cast<i32>(a1.m_brd[i][1]);
            if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                continue;
            }
            if (!try_enq_out(map, w, h, sx, sy, static_cast<u16>(px), static_cast<u16>(py), id, &q)) {
                break;
            }
        }
    }
    static const i16 k_dx[4] = {-1, 1, 0, 0};
    static const i16 k_dy[4] = {0, 0, -1, 1};
    u16 cx = 0;
    u16 cy = 0;
    while (cq_pop(&q, &cx, &cy)) {
        if (!riv_done && map.get_river(cx, cy) != 0) {
            riv_done = true;
            if (flood_river(map, w, h, sx, sy, cx, cy, player, live, best)) {
                return settled + 1u;
            }
        }
        const i32 dx = static_cast<i32>(cx) - static_cast<i32>(sx);
        const i32 dy = static_cast<i32>(cy) - static_cast<i32>(sy);
        if (past_near(dx, dy) && settle_ok(map, cx, cy, player)) {
            settled = settled + 1u;
            if (is_premium(map, w, h, cx, cy)) {
                if (take_pt(map, live, cx, cy)) {
                    return settled;
                }
            } else {
                note_best(map, live, cx, cy, best);
            }
        }
        if (live->pts.n >= SMS_BEST_N) {
            return settled;
        }
        for (u16 d = 0; d < 4u; ++d) {
            const i32 nx = static_cast<i32>(cx) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(cy) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            if (!try_enq_out(map, w, h, sx, sy, static_cast<u16>(nx), static_cast<u16>(ny), id, &q)) {
                break;
            }
        }
    }
    return settled;
}

//================================================================================================================================
//=> - SenseSettlingPtsOpt -
//================================================================================================================================

SmSettlerBestPts SenseSettlingPtsOpt::select_and_pick_pts (
    const GameArraySimple& map,
    CityArray& cities,
    u16 player)
{
    LivePicks live = {};
    live.pts.n = 0;
    live.excl = nullptr;
    for (u16 i = 0; i < SMS_BEST_N; ++i) {
        live.pts.pts[i].x = U16_KEY_NULL;
        live.pts.pts[i].y = U16_KEY_NULL;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0 || h == 0) {
        return live.pts;
    }
    Whiteboard_1B excl("SenseSettlingPtsOpt", "select_and_pick_pts", 0u);
    if (!excl.ok()) {
        return live.pts;
    }
    std::memset(excl.raw(), 0, static_cast<size_t>(WhiteboardMng::tile_n()));
    live.excl = &excl;
    const u16 cn = cities.get_city_count();
    bool any_src = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c == nullptr || c->get_owner() != player || !c->is_frontier()) {
            continue;
        }
        any_src = true;
        break;
    }
    if (!any_src) {
        return live.pts;
    }
    BestNp spill[SMS_BEST_N];
    u16 spill_n = 0;
    for (u16 i = 0; i < cn; ++i) {
        City* c = cities.get_city(i);
        if (c == nullptr || c->get_owner() != player || !c->is_frontier()) {
            continue;
        }
        const u16 sx = c->get_x();
        const u16 sy = c->get_y();
        if (sx >= w || sy >= h) {
            c->city_no_longer_frontier();
            continue;
        }
        BestNp best = {};
        best.x = U16_KEY_NULL;
        best.y = U16_KEY_NULL;
        best.score = 0;
        const u32 settled = flood_city(map, w, h, sx, sy, player, &live, &best);
        if (settled == 0 && best.x == U16_KEY_NULL) {
            c->city_no_longer_frontier();
            continue;
        }
        if (best.x != U16_KEY_NULL && spill_n < SMS_BEST_N) {
            spill[spill_n] = best;
            spill_n = static_cast<u16>(spill_n + 1u);
        }
        if (live.pts.n >= SMS_BEST_N) {
            return live.pts;
        }
    }
    if (live.pts.n < SMS_BEST_N) {
        for (u16 i = 0; i < spill_n; ++i) {
            take_pt(map, &live, spill[i].x, spill[i].y);
            if (live.pts.n >= SMS_BEST_N) {
                break;
            }
        }
    }
    return live.pts;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
