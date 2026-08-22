//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_connector.h"
#include "assert_log.h"
#include "city.h"
#include "city_tile_manager.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

#define CC_SPINE_MAX 128u
#define CC_LINK_SLOTS 32768u

//================================================================================================================================
//=> - State -
//================================================================================================================================

static GameState* g_st = nullptr;

static const i32 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const u8 k_dir_n = 8u;
static const u8 k_win_r = 20u;
static const u16 k_win = static_cast<u16>(k_win_r * 2u + 1u);
static const u16 k_win_n = static_cast<u16>(k_win * k_win);

struct CcSpine {
    u16 m_x[CC_SPINE_MAX];
    u16 m_y[CC_SPINE_MAX];
    u16 m_n;
};

static CcSpine g_spine[CC_LINK_SLOTS];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 link_slot (u16 home_idx, u8 dir) {
    return static_cast<u32>(home_idx) * 4u + static_cast<u32>(dir);
}

static CcSpine* spine_get (u16 home_idx, u8 dir) {
    const u32 slot = link_slot(home_idx, dir);
    if (slot >= CC_LINK_SLOTS) {
        return nullptr;
    }
    return &g_spine[slot];
}

static void spine_clr_all () {
    for (u32 i = 0; i < CC_LINK_SLOTS; ++i) {
        g_spine[i].m_n = 0;
    }
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static u32 cheb (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u32 adx = static_cast<u32>(dx < 0 ? -dx : dx);
    const u32 ady = static_cast<u32>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static bool in_bounds (const GameState& state, u16 x, u16 y) {
    return x < state.m_map.width() && y < state.m_map.height();
}

static bool tile_block (const GameState& state, u16 x, u16 y) {
    if (!in_bounds(state, x, y)) {
        return true;
    }
    const u8 terr = state.m_map.get_terrain(x, y);
    if (overlay_is_water_terr(terr)) {
        return true;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return true;
    }
    return false;
}

static bool try_step (GameState& state, u16 unit_idx, u16 ux, u16 uy) {
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    if (!UnitMovementMng::can_step(state, key, ux, uy, &cost)) {
        return false;
    }
    return UnitMovementMng::apply_step(state, key, ux, uy);
}

static void promote_virtual (GameState& state, u16 x, u16 y) {
    GameTileSimple* t = state.m_map.tile(x, y);
    if (road_is_virtual(static_cast<u8>(t->m_road_typ))) {
        t->m_road_typ = ROAD_PATH;
    }
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

static bool line_clear (const GameState& state, u16 x0, u16 y0, u16 x1, u16 y1) {
    u16 x = x0;
    u16 y = y0;
    while (x != x1 || y != y1) {
        u16 nx = 0;
        u16 ny = 0;
        if (!step_linear(x, y, x1, y1, &nx, &ny)) {
            return false;
        }
        if (tile_block(state, nx, ny)) {
            return false;
        }
        x = nx;
        y = ny;
    }
    return true;
}

static bool step_flood (
    const GameState& state,
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
    const u16 mw = state.m_map.width();
    const u16 mh = state.m_map.height();
    const u32 tidx = static_cast<u32>(tly) * k_win + static_cast<u32>(tlx);
    wd[tidx] = 0;
    u32 qn = 0;
    qx[qn] = static_cast<u8>(tlx);
    qy[qn] = static_cast<u8>(tly);
    ++qn;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u8 lx = qx[qh];
        const u8 ly = qy[qh];
        const u16 d0 = wd[static_cast<u32>(ly) * k_win + static_cast<u32>(lx)];
        for (u8 dir = 0; dir < k_dir_n; ++dir) {
            const i32 nlx = static_cast<i32>(lx) + k_dx[dir];
            const i32 nly = static_cast<i32>(ly) + k_dy[dir];
            if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(k_win) || nly >= static_cast<i32>(k_win)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(nly) * k_win + static_cast<u32>(nlx);
            if (wd[nidx] != U16_KEY_NULL) {
                continue;
            }
            const i32 nx = ox0 + nlx;
            const i32 ny = oy0 + nly;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(mw) || ny >= static_cast<i32>(mh)) {
                continue;
            }
            if (tile_block(state, static_cast<u16>(nx), static_cast<u16>(ny))) {
                continue;
            }
            wd[nidx] = static_cast<u16>(d0 + 1u);
            qx[qn] = static_cast<u8>(nlx);
            qy[qn] = static_cast<u8>(nly);
            ++qn;
        }
    }
    const u32 widx = static_cast<u32>(wly) * k_win + static_cast<u32>(wlx);
    if (wd[widx] == U16_KEY_NULL || wd[widx] == 0) {
        return false;
    }
    u16 best_d = wd[widx];
    bool hit = false;
    u16 bx = wx;
    u16 by = wy;
    for (u8 dir = 0; dir < k_dir_n; ++dir) {
        const i32 nx = static_cast<i32>(wx) + k_dx[dir];
        const i32 ny = static_cast<i32>(wy) + k_dy[dir];
        if (nx < 0 || ny < 0) {
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
    const GameState& state,
    u16 wx,
    u16 wy,
    u16 tx,
    u16 ty,
    u16* ox,
    u16* oy)
{
    if (line_clear(state, wx, wy, tx, ty)) {
        return step_linear(wx, wy, tx, ty, ox, oy);
    }
    return step_flood(state, wx, wy, tx, ty, ox, oy);
}

static void build_spine (
    GameState& state,
    u16 x0,
    u16 y0,
    u16 x1,
    u16 y1,
    CcSpine* sp)
{
    sp->m_n = 0;
    u16 x = x0;
    u16 y = y0;
    for (;;) {
        if (sp->m_n < CC_SPINE_MAX) {
            sp->m_x[sp->m_n] = x;
            sp->m_y[sp->m_n] = y;
            sp->m_n = static_cast<u16>(sp->m_n + 1u);
        }
        if (x == x1 && y == y1) {
            break;
        }
        u16 nx = 0;
        u16 ny = 0;
        if (!pick_step(state, x, y, x1, y1, &nx, &ny)) {
            break;
        }
        x = nx;
        y = ny;
    }
}

static void stamp_spine (GameState& state, const CcSpine* sp) {
    for (u16 i = 0; i < sp->m_n; ++i) {
        GameTileSimple* t = state.m_map.tile(sp->m_x[i], sp->m_y[i]);
        if (t->m_road_typ == ROAD_NONE) {
            t->m_road_typ = ROAD_VIRTUAL;
        }
    }
}

static bool spine_has_virtual (const GameState& state, const CcSpine* sp) {
    if (sp == nullptr) {
        return false;
    }
    for (u16 i = 0; i < sp->m_n; ++i) {
        if (road_is_virtual(state.m_map.get_road_typ(sp->m_x[i], sp->m_y[i]))) {
            return true;
        }
    }
    return false;
}

static bool spine_all_built (const GameState& state, const CcSpine* sp) {
    if (sp == nullptr) {
        return true;
    }
    for (u16 i = 0; i < sp->m_n; ++i) {
        if (road_is_virtual(state.m_map.get_road_typ(sp->m_x[i], sp->m_y[i]))) {
            return false;
        }
    }
    return true;
}

static bool spine_nearest_virtual (
    const GameState& state,
    u16 wx,
    u16 wy,
    const CcSpine* sp,
    u16* ox,
    u16* oy)
{
    if (sp == nullptr) {
        return false;
    }
    u32 best_d = UINT32_MAX;
    bool hit = false;
    u16 bx = 0;
    u16 by = 0;
    for (u16 i = 0; i < sp->m_n; ++i) {
        const u16 x = sp->m_x[i];
        const u16 y = sp->m_y[i];
        if (!road_is_virtual(state.m_map.get_road_typ(x, y))) {
            continue;
        }
        const u32 d = cheb(wx, wy, x, y);
        if (d < best_d) {
            best_d = d;
            bx = x;
            by = y;
            hit = true;
        }
    }
    if (!hit) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

static void claim_link (City* home, u8 hdir, City* tgt, u16 home_idx) {
    home->conn_city_is_locked(hdir);
    for (u8 d = 0; d < 4u; ++d) {
        if (tgt->get_conn_city(d) == home_idx) {
            tgt->conn_city_is_locked(d);
            return;
        }
    }
}

static void mark_built (City* home, u8 hdir, City* tgt, u16 home_idx) {
    home->conn_city_is_built(hdir);
    home->conn_city_is_locked(hdir);
    for (u8 d = 0; d < 4u; ++d) {
        if (tgt->get_conn_city(d) == home_idx) {
            tgt->conn_city_is_built(d);
            tgt->conn_city_is_locked(d);
            break;
        }
    }
    CcSpine* sp = spine_get(home_idx, hdir);
    if (sp != nullptr) {
        sp->m_n = 0;
    }
}

static void arm_city_roads (City* city, u16 city_idx) {
    if (city == nullptr) {
        return;
    }
    for (u8 d = 0; d < 4u; ++d) {
        const u16 j = city->get_conn_city(d);
        if (j == U16_KEY_NULL || j <= city_idx) {
            continue;
        }
        if (city->is_conn_city_built(d)) {
            continue;
        }
        city->set_city_has_worker(1);
        return;
    }
}

static bool city_has_road_pending (const GameState& state, u16 home_idx, const City* home) {
    if (home == nullptr || !state.m_city_net.is_valid()) {
        return false;
    }
    for (u8 d = 0; d < 4u; ++d) {
        const u16 j = home->get_conn_city(d);
        if (j == U16_KEY_NULL || j <= home_idx) {
            continue;
        }
        if (home->is_conn_city_built(d)) {
            continue;
        }
        if (!home->is_conn_city_locked(d)) {
            return true;
        }
        const CcSpine* sp = spine_get(home_idx, d);
        if (sp != nullptr && spine_has_virtual(state, sp)) {
            return true;
        }
    }
    return false;
}

static CcSpine* ensure_stamped (GameState& state, City* home, u8 hdir, u16 home_idx, City* tgt) {
    CcSpine* sp = spine_get(home_idx, hdir);
    if (sp == nullptr) {
        return nullptr;
    }
    if (home->is_conn_city_locked(hdir) && sp->m_n != 0u) {
        return sp;
    }
    claim_link(home, hdir, tgt, home_idx);
    build_spine(state, home->get_x(), home->get_y(), tgt->get_x(), tgt->get_y(), sp);
    stamp_spine(state, sp);
    home->set_city_has_worker(1);
    return sp;
}

static bool pick_link_virtual (
    GameState& state,
    u16 home_idx,
    City* home,
    u16 wx,
    u16 wy,
    u16* ox,
    u16* oy,
    u16* out_tgt,
    u8* out_dir)
{
    u32 best_d = UINT32_MAX;
    bool hit = false;
    for (u8 d = 0; d < 4u; ++d) {
        const u16 j = home->get_conn_city(d);
        if (j == U16_KEY_NULL || j <= home_idx) {
            continue;
        }
        if (home->is_conn_city_built(d)) {
            continue;
        }
        City* tgt = state.m_cities.get_city(j);
        if (tgt == nullptr) {
            continue;
        }
        CcSpine* sp = ensure_stamped(state, home, d, home_idx, tgt);
        if (sp == nullptr) {
            continue;
        }
        u16 vx = 0;
        u16 vy = 0;
        if (!spine_nearest_virtual(state, wx, wy, sp, &vx, &vy)) {
            continue;
        }
        const u32 dist = cheb(wx, wy, vx, vy);
        if (dist < best_d) {
            best_d = dist;
            *ox = vx;
            *oy = vy;
            *out_tgt = j;
            *out_dir = d;
            hit = true;
        }
    }
    return hit;
}

void CityConnector::on_city_net_changed (GameState& state, u16 city_idx) {
    City* city = state.m_cities.get_city(city_idx);
    arm_city_roads(city, city_idx);
    for (u16 i = 0; i < city_idx; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            if (c->get_conn_city(d) != city_idx) {
                continue;
            }
            if (c->is_conn_city_built(d)) {
                break;
            }
            c->set_city_has_worker(1);
            break;
        }
    }
}

void CityConnector::clear_idle_flag (
    GameState& state,
    u16 city_idx,
    City* city,
    u16 cx,
    u16 cy,
    bool imp_disk_done)
{
    if (city == nullptr || !imp_disk_done) {
        return;
    }
    if (CityTileManager::count_worked(cx, cy, city_idx) == 0u) {
        return;
    }
    if (city_has_road_pending(state, city_idx, city)) {
        return;
    }
    city->set_city_has_worker(0);
}

bool CityConnector::has_virtual_at (const GameState& state, u16 x, u16 y) {
    return road_is_virtual(state.m_map.get_road_typ(x, y));
}

//================================================================================================================================
//=> - CityConnector -
//================================================================================================================================

bool CityConnector::begin (GameState& state) {
    clear();
    if (state.m_player_states == nullptr || state.m_player_n == 0) {
        return false;
    }
    if (state.m_statics == nullptr) {
        return false;
    }
    if (!UnitMovementMng::mvt_ready() && !UnitMovementMng::setup_mvt_costs(*state.m_statics)) {
        return false;
    }
    if (state.m_map.width() == 0 || state.m_map.height() == 0) {
        return false;
    }
    state.m_city_net.clear();
    if (!state.m_city_net.begin(state.m_cities, state.m_map)) {
        return false;
    }
    const u16 n = state.m_cities.get_city_count();
    for (u16 i = 0; i < n; ++i) {
        if (!state.m_city_net.add(i)) {
            state.m_city_net.clear();
            return false;
        }
    }
    g_st = &state;
    return true;
}

void CityConnector::clear () {
    spine_clr_all();
    if (g_st != nullptr) {
        g_st->m_city_net.clear();
    }
    g_st = nullptr;
}

bool CityConnector::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(state.m_city_net.is_valid(), "CityConnector handle got invalid city network");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "CityConnector handle got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "CityConnector handle unit has null x");
    const u16 home_idx = WorkerHelper::get_data(unit);
    City* home = state.m_cities.get_city(home_idx);
    GAME_EXPECT(home != nullptr, "CityConnector handle got nullptr home city");
    u16 tgt_idx = U16_KEY_NULL;
    u8 hdir = 0;
    u16 vx = 0;
    u16 vy = 0;
    if (!pick_link_virtual(state, home_idx, home, unit->m_x, unit->m_y, &vx, &vy, &tgt_idx, &hdir)) {
        return false;
    }
    City* tgt = state.m_cities.get_city(tgt_idx);
    GAME_EXPECT(tgt != nullptr, "CityConnector handle got nullptr tgt city");
    CcSpine* sp = spine_get(home_idx, hdir);
    if (unit->m_x == vx && unit->m_y == vy) {
        promote_virtual(state, vx, vy);
        if (sp != nullptr && spine_all_built(state, sp)) {
            mark_built(home, hdir, tgt, home_idx);
        }
        return true;
    }
    u16 nx = 0;
    u16 ny = 0;
    if (!pick_step(state, unit->m_x, unit->m_y, vx, vy, &nx, &ny)) {
        return false;
    }
    if (tile_block(state, nx, ny)) {
        return false;
    }
    if (!try_step(state, unit_idx, nx, ny)) {
        return false;
    }
    if (unit->m_x == vx && unit->m_y == vy) {
        promote_virtual(state, vx, vy);
        if (sp != nullptr && spine_all_built(state, sp)) {
            mark_built(home, hdir, tgt, home_idx);
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
