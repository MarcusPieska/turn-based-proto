//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_river_mk2.h"
#include "game_map_defs.h"
#include "game_map_grid_defs.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - Shared helpers -
//================================================================================================================================

static bool riv_path (const GameArraySimple& map, u16 x, u16 y) {
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

static bool riv_unexp (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 sight,
    u16 x,
    u16 y) {
    const u16 w = map.width();
    const u16 h = map.height();
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
            if (ax >= w || ay >= h || exp.get(ax, ay) != 0u) {
                continue;
            }
            return true;
        }
    }
    return false;
}

static bool riv_front (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 sight,
    u16 x,
    u16 y) {
    return map.get_river(x, y) != 0u && riv_unexp(map, exp, sight, x, y);
}

static bool riv_pick (
    const GameArraySimple& map,
    const MapBitOverlay& exp,
    u16 sight,
    u16 x,
    u16 y) {
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
        if (riv_front(map, exp, sight, nx, ny)) {
            return true;
        }
    }
    return false;
}

static bool l2g_win (u16 wx, u16 wy, u16 lx, u16 ly, u16& gx, u16& gy) {
    const i32 gx_i = static_cast<i32>(lx) - 7 + static_cast<i32>(wx);
    const i32 gy_i = static_cast<i32>(ly) - 7 + static_cast<i32>(wy);
    if (gx_i < 0 || gy_i < 0) {
        return false;
    }
    gx = static_cast<u16>(gx_i);
    gy = static_cast<u16>(gy_i);
    return true;
}

static bool fill_grad (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 sight,
    u16 wx,
    u16 wy,
    u8* dst) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u8 k_blk = 255u;
    const u8 k_non = 254u;
    const u8 k_wat = 253u;
    u16 q[225];
    u16 qh = 0u;
    u16 qt = 0u;
    bool fog = false;
    for (u16 ly = 0u; ly < 15u; ++ly) {
        for (u16 lx = 0u; lx < 15u; ++lx) {
            const u16 i = static_cast<u16>(ly * 15u + lx);
            u16 gx = 0u;
            u16 gy = 0u;
            if (!l2g_win(wx, wy, lx, ly, gx, gy) || gx >= w || gy >= h || !riv_path(map, gx, gy)) {
                dst[i] = k_blk;
                continue;
            }
            if (exp.get(gx, gy) != 0u) {
                dst[i] = k_non;
                continue;
            }
            if (riv_front(map, exp, sight, gx, gy)) {
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
        const u16 clx = static_cast<u16>(ci % 15u);
        const u16 cly = static_cast<u16>(ci / 15u);
        const u8 cd = dst[ci];
        for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
            const i32 nxi = static_cast<i32>(clx) + MAP_NBR8_DX[k];
            const i32 nyi = static_cast<i32>(cly) + MAP_NBR8_DY[k];
            if (nxi < 0 || nyi < 0) {
                continue;
            }
            const u16 nlx = static_cast<u16>(nxi);
            const u16 nly = static_cast<u16>(nyi);
            if (nlx >= 15u || nly >= 15u) {
                continue;
            }
            const u16 ni = static_cast<u16>(nly * 15u + nlx);
            if (dst[ni] != k_wat && dst[ni] != k_non) {
                continue;
            }
            u16 ngx = 0u;
            u16 ngy = 0u;
            if (!l2g_win(wx, wy, nlx, nly, ngx, ngy) || ngx >= w || ngy >= h || !riv_path(map, ngx, ngy)) {
                continue;
            }
            dst[ni] = static_cast<u8>(cd + 1u);
            q[qt++] = ni;
        }
    }
    const u16 ci = static_cast<u16>(7u * 15u + 7u);
    return dst[ci] < k_wat;
}

static void reveal_cheb (
    MapBitOverlay& exp,
    u16 sight,
    u8 player,
    u16 px,
    u16 py,
    u16 ox,
    u16 oy,
    bool delta) {
    const u16 w = exp.width();
    const u16 h = exp.height();
    for (i16 dy = -static_cast<i16>(sight); dy <= static_cast<i16>(sight); ++dy) {
        for (i16 dx = -static_cast<i16>(sight); dx <= static_cast<i16>(sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(px) + dx;
            const i32 yi = static_cast<i32>(py) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h) {
                continue;
            }
            if (delta) {
                const i32 oxo = static_cast<i32>(x) - static_cast<i32>(ox);
                const i32 oyo = static_cast<i32>(y) - static_cast<i32>(oy);
                const i32 oax = (oxo < 0) ? -oxo : oxo;
                const i32 oay = (oyo < 0) ? -oyo : oyo;
                if (static_cast<u32>((oax > oay) ? oax : oay) <= static_cast<u32>(sight)) {
                    continue;
                }
            }
            if (exp.get(x, y) != 0u) {
                continue;
            }
            exp.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(player)));
        }
    }
}

u16 WalkRiverMk2::can_ai_start_from (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 x,
    u16 y,
    u16 sight) {
    if (map.get_river(x, y) != 0u && riv_pick(map, exp, sight, x, y)) {
        return 1u;
    }
    u8 dst[225];
    if (riv_path(map, x, y) && fill_grad(map, exp, sight, x, y, dst)) {
        return 2u;
    }
    return 0u;
}

//================================================================================================================================
//=> - WalkRiverMk2 -
//================================================================================================================================

WalkRiverMk2::WalkRiverMk2 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player) :
    m_map(map),
    m_exp(explored),
    m_pos{sx, sy},
    m_win{sx, sy},
    m_sight(sight),
    m_player(player),
    m_ph(k_ph_done),
    m_greedy_ph(false),
    m_has_blk(false),
    m_blk{0, 0},
    m_hn(0),
    m_hp(0),
    m_bck_n(0),
    m_bck_t(0) {
    if (map.get_river(sx, sy) != 0u && riv_pick(map, explored, sight, sx, sy)) {
        start_greedy_ph();
    } else if (riv_path(map, sx, sy) && fill_grad(map, explored, sight, sx, sy, m_hist.dst)) {
        m_win = m_pos;
        m_ph = k_ph_repos;
    }
    reveal_around();
}

void WalkRiverMk2::hist_push (MapPt p) {
    m_hist.pt[m_hp] = p;
    m_hp = (m_hp + 1u) % k_ring;
    if (m_hn < k_ring) {
        ++m_hn;
    }
}

bool WalkRiverMk2::hist_pop (MapPt& p) {
    if (m_hn == 0u) {
        return false;
    }
    m_hp = (m_hp + k_ring - 1u) % k_ring;
    --m_hn;
    p = m_hist.pt[m_hp];
    return true;
}

void WalkRiverMk2::bck_clr () {
    m_bck_n = 0;
    m_bck_t = 0;
}

void WalkRiverMk2::bck_push (MapPt p) {
    if (bck_has(p)) {
        return;
    }
    m_bck[m_bck_t] = p;
    m_bck_t = (m_bck_t + 1u) % k_q;
    if (m_bck_n < k_q) {
        ++m_bck_n;
    }
}

bool WalkRiverMk2::bck_has (MapPt p) const {
    for (u16 i = 0; i < m_bck_n; ++i) {
        const u16 j = static_cast<u16>((m_bck_t + k_q - m_bck_n + i) % k_q);
        if (m_bck[j].x == p.x && m_bck[j].y == p.y) {
            return true;
        }
    }
    return false;
}

bool WalkRiverMk2::hist8_has (MapPt p) const {
    const u16 n = (m_hn < k_q) ? m_hn : k_q;
    for (u16 i = 0; i < n; ++i) {
        const u16 j = static_cast<u16>((m_hp + k_ring - 1u - i) % k_ring);
        if (m_hist.pt[j].x == p.x && m_hist.pt[j].y == p.y) {
            return true;
        }
    }
    return false;
}

bool WalkRiverMk2::blk_peek (MapPt p) const {
    return hist8_has(p) || bck_has(p);
}

u16 WalkRiverMk2::lidx (u16 lx, u16 ly) const {
    return static_cast<u16>(ly * k_box + lx);
}

bool WalkRiverMk2::g2l (u16 gx, u16 gy, u16& lx, u16& ly) const {
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

bool WalkRiverMk2::l2g (u16 lx, u16 ly, u16& gx, u16& gy) const {
    return l2g_win(m_win.x, m_win.y, lx, ly, gx, gy);
}

bool WalkRiverMk2::is_riv_front (u16 x, u16 y) const {
    return riv_front(m_map, m_exp, m_sight, x, y);
}

bool WalkRiverMk2::pick_near_front (u16 sx, u16 sy, u16& ox, u16& oy) const {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    const bool use_blk = m_has_blk && m_ph == k_ph_greedy;
    const bool use4 = m_ph == k_ph_greedy;
    for (u32 k = 0u; k < (use4 ? 4u : 8u); ++k) {
        const i32 nx = static_cast<i32>(sx) + (use4 ? MAP_NBR4_DX[k] : MAP_NBR8_DX[k]);
        const i32 ny = static_cast<i32>(sy) + (use4 ? MAP_NBR4_DY[k] : MAP_NBR8_DY[k]);
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h) {
            continue;
        }
        if (use_blk && cx == m_blk.x && cy == m_blk.y) {
            continue;
        }
        if (bck_has({cx, cy})) {
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

bool WalkRiverMk2::peek_branch (u16 jx, u16 jy, u16 sx, u16 sy) const {
    MapPt vv[k_pkq];
    u16 vn = 0;
    MapPt stk[k_pkq];
    u16 sn = 0;
    stk[0] = MapPt{sx, sy};
    sn = 1u;
    while (sn > 0u) {
        --sn;
        const u16 cx = stk[sn].x;
        const u16 cy = stk[sn].y;
        if (riv_unexp(m_map, m_exp, m_sight, cx, cy)) {
            return true;
        }
        if (vn >= k_pkq) {
            continue;
        }
        bool dup = false;
        for (u16 vi = 0; vi < vn; ++vi) {
            if (vv[vi].x == cx && vv[vi].y == cy) {
                dup = true;
                break;
            }
        }
        if (dup) {
            continue;
        }
        vv[vn] = MapPt{cx, cy};
        ++vn;
        const u16 w = m_map.width();
        const u16 h = m_map.height();
        for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
            const i32 nx = static_cast<i32>(cx) + MAP_NBR8_DX[k];
            const i32 ny = static_cast<i32>(cy) + MAP_NBR8_DY[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ax = static_cast<u16>(nx);
            const u16 ay = static_cast<u16>(ny);
            if (ax >= w || ay >= h || m_map.get_river(ax, ay) == 0u) {
                continue;
            }
            if (ax == jx && ay == jy) {
                continue;
            }
            if (blk_peek({ax, ay})) {
                continue;
            }
            bool seen = false;
            for (u16 vi = 0; vi < vn; ++vi) {
                if (vv[vi].x == ax && vv[vi].y == ay) {
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
            stk[sn] = MapPt{ax, ay};
            ++sn;
        }
    }
    return false;
}

bool WalkRiverMk2::try_peek_branch (bool cont) {
    const u16 jx = m_pos.x;
    const u16 jy = m_pos.y;
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (u32 k = 0u; k < MAP_NBR8_N; ++k) {
        const i32 nx = static_cast<i32>(jx) + MAP_NBR8_DX[k];
        const i32 ny = static_cast<i32>(jy) + MAP_NBR8_DY[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 cx = static_cast<u16>(nx);
        const u16 cy = static_cast<u16>(ny);
        if (cx >= w || cy >= h || m_map.get_river(cx, cy) == 0u) {
            continue;
        }
        if (cont && m_has_blk && cx == m_blk.x && cy == m_blk.y) {
            continue;
        }
        if (blk_peek({cx, cy})) {
            continue;
        }
        if (!peek_branch(jx, jy, cx, cy)) {
            continue;
        }
        const u16 ox = jx;
        const u16 oy = jy;
        hist_push({ox, oy});
        m_pos.x = cx;
        m_pos.y = cy;
        reveal_delta({ox, oy}, m_pos);
        m_blk.x = ox;
        m_blk.y = oy;
        m_has_blk = true;
        if (cont) {
            m_ph = k_ph_greedy;
            m_greedy_ph = true;
        } else {
            start_greedy_ph();
            m_blk.x = ox;
            m_blk.y = oy;
            m_has_blk = true;
        }
        return true;
    }
    return false;
}

void WalkRiverMk2::start_greedy_ph () {
    m_ph = k_ph_greedy;
    m_greedy_ph = true;
    m_has_blk = false;
    bck_clr();
}

bool WalkRiverMk2::try_resume_greedy () {
    u16 ox = 0u;
    u16 oy = 0u;
    if (!pick_near_front(m_pos.x, m_pos.y, ox, oy)) {
        return false;
    }
    start_greedy_ph();
    return true;
}

void WalkRiverMk2::end_greedy_local () {
    m_greedy_ph = false;
    m_has_blk = false;
    if (m_hn == 0u) {
        if (!try_resume_greedy() && !try_peek_branch(false)) {
            end_all();
        }
        return;
    }
    bck_push(m_pos);
    m_ph = k_ph_back;
}

void WalkRiverMk2::end_all () {
    m_greedy_ph = false;
    m_has_blk = false;
    bck_clr();
    m_ph = k_ph_done;
}

void WalkRiverMk2::reveal_around () {
    reveal_cheb(m_exp, m_sight, m_player, m_pos.x, m_pos.y, 0u, 0u, false);
}

void WalkRiverMk2::reveal_delta (MapPt o, MapPt n) {
    reveal_cheb(m_exp, m_sight, m_player, n.x, n.y, o.x, o.y, true);
}

bool WalkRiverMk2::step_repos () {
    u16 lx = 0u;
    u16 ly = 0u;
    if (!g2l(m_pos.x, m_pos.y, lx, ly)) {
        return false;
    }
    const u16 ci = lidx(lx, ly);
    const u8 cur = m_hist.dst[ci];
    if (cur >= k_wat) {
        return false;
    }
    u8 best = cur;
    MapPt bp = m_pos;
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
        const u8 nd = m_hist.dst[lidx(nlx, nly)];
        if (nd >= k_wat || nd >= cur) {
            continue;
        }
        u16 gx = 0u;
        u16 gy = 0u;
        if (!l2g(nlx, nly, gx, gy) || !riv_path(m_map, gx, gy)) {
            continue;
        }
        if (nd < best) {
            best = nd;
            bp.x = gx;
            bp.y = gy;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    const MapPt o = m_pos;
    m_pos = bp;
    reveal_delta(o, m_pos);
    return true;
}

bool WalkRiverMk2::step_greedy () {
    if (!m_greedy_ph) {
        return false;
    }
    u16 nx = 0;
    u16 ny = 0;
    if (pick_near_front(m_pos.x, m_pos.y, nx, ny)) {
        const u16 ox = m_pos.x;
        const u16 oy = m_pos.y;
        hist_push({ox, oy});
        m_blk.x = ox;
        m_blk.y = oy;
        m_has_blk = true;
        m_pos.x = nx;
        m_pos.y = ny;
        reveal_delta({ox, oy}, m_pos);
        return true;
    }
    if (try_peek_branch(true)) {
        return true;
    }
    end_greedy_local();
    return false;
}

bool WalkRiverMk2::step_back () {
    if (try_resume_greedy()) {
        return false;
    }
    if (try_peek_branch(false)) {
        return false;
    }
    MapPt tp{0, 0};
    if (!hist_pop(tp)) {
        end_all();
        return false;
    }
    const u16 ox = m_pos.x;
    const u16 oy = m_pos.y;
    m_pos = tp;
    reveal_delta({ox, oy}, m_pos);
    bck_push({ox, oy});
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

void WalkRiverMk2::reveal () {
    reveal_around();
}

bool WalkRiverMk2::pump () {
    if (m_ph == k_ph_repos) {
        const bool acted = step_repos();
        if (acted && m_map.get_river(m_pos.x, m_pos.y) != 0u) {
            m_hn = 0u;
            m_hp = 0u;
            start_greedy_ph();
        }
        return acted;
    }
    if (m_ph == k_ph_back) {
        return step_back();
    }
    if (m_ph == k_ph_greedy) {
        return step_greedy();
    }
    return false;
}

void WalkRiverMk2::finish_move_credit (u16 loop, bool acted) {
    if (loop >= k_pump && m_ph != k_ph_done) {
        end_all();
    } else if (m_ph == k_ph_repos && !acted) {
        end_all();
    }
}

void WalkRiverMk2::move (u16 moves) {
    if (m_ph == k_ph_done) {
        return;
    }
    reveal();
    for (u16 m = 0; m < moves; ++m) {
        if (m_ph == k_ph_done) {
            break;
        }
        bool acted = false;
        u16 loop = 0u;
        while (!acted && m_ph != k_ph_done && loop < k_pump) {
            ++loop;
            acted = pump();
        }
        finish_move_credit(loop, acted);
    }
}

bool WalkRiverMk2::greedy_ph () const {
    return m_greedy_ph;
}

u8 WalkRiverMk2::phase () const {
    return m_ph;
}

u16 WalkRiverMk2::x () const {
    return m_pos.x;
}

u16 WalkRiverMk2::y () const {
    return m_pos.y;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
