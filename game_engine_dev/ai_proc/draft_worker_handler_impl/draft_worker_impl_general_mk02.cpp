//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "assert_log.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_tile_manager.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "std_add_helper.h"
#include "tile_working.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - State -
//================================================================================================================================

static bool g_ok = false;
static GameState* g_st = nullptr;
static u16 g_rad = 2u;

static const i32 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const u8 k_dir_n = 8u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool in_bounds (const GameState& state, u16 x, u16 y) {
    return x < state.m_map.width() && y < state.m_map.height();
}

static u16 cheb (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
    const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static u16 work_rad () {
    const CircArea area = CityTileManager::work_area();
    u16 rad = 0;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 ax = area.m_brd[i][0] < 0 ? -area.m_brd[i][0] : area.m_brd[i][0];
        const i32 ay = area.m_brd[i][1] < 0 ? -area.m_brd[i][1] : area.m_brd[i][1];
        const u16 c = static_cast<u16>(ax > ay ? ax : ay);
        if (c > rad) {
            rad = c;
        }
    }
    return rad == 0 ? 2u : rad;
}

static bool home_xy (const GameState& state, const UnitAddStruct* unit, u16* ox, u16* oy, u16* city_idx) {
    const u16 idx = WorkerHelper::get_data(unit);
    const City* c = state.m_cities.get_city(idx);
    if (c == nullptr || c->get_owner() != unit->m_player_idx) {
        return false;
    }
    *ox = c->get_x();
    *oy = c->get_y();
    *city_idx = idx;
    return true;
}

static bool in_disk (u16 x, u16 y, u16 cx, u16 cy) {
    return cheb(x, y, cx, cy) <= g_rad;
}

static bool is_owned (const GameState& state, u16 x, u16 y, u16 player) {
    return state.m_map.get_civ_owner(x, y) == static_cast<u8>(player);
}

static bool is_farm_cand (
    const GameState& state,
    u16 x,
    u16 y,
    u16 player,
    u16 city_idx,
    u16 cx,
    u16 cy)
{
    if (!in_bounds(state, x, y) || !in_disk(x, y, cx, cy)) {
        return false;
    }
    if (!is_owned(state, x, y, player)) {
        return false;
    }
    if (TileWorking::get_worker(x, y) != city_idx) {
        return false;
    }
    if (state.m_map.get_terrain(x, y) != TERR_PLAINS[0]) {
        return false;
    }
    if (state.m_map.get_add_typ(x, y) != BUILD_ADD_STD) {
        return false;
    }
    if (state.m_map.get_add_idx(x, y) != U16_KEY_NULL && StdAddHelper::has_farm(state.m_map.tile(x, y))) {
        return false;
    }
    return true;
}

static bool try_plant (GameState& state, u16 x, u16 y, u16 player, u16 city_idx, u16 cx, u16 cy) {
    if (!is_farm_cand(state, x, y, player, city_idx, cx, cy)) {
        return false;
    }
    if (!state.m_map.set_tile_add(x, y, 0u, BUILD_ADD_STD)) {
        return false;
    }
    StdAddHelper::set_farm(state.m_map.tile(x, y));
    return true;
}

static bool try_step (GameState& state, u16 unit_idx, u16 ux, u16 uy, u16 cx, u16 cy) {
    if (!in_disk(ux, uy, cx, cy)) {
        return false;
    }
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    if (!UnitMovementMng::can_step(state, key, ux, uy, &cost)) {
        return false;
    }
    return UnitMovementMng::apply_step(state, key, ux, uy);
}

static bool can_try_step (GameState& state, u16 unit_idx, u16 ux, u16 uy, u16 cx, u16 cy) {
    if (!in_disk(ux, uy, cx, cy)) {
        return false;
    }
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    return UnitMovementMng::can_step(state, key, ux, uy, &cost);
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool has_progress_step (
    GameState& state,
    u16 unit_idx,
    u16 ox,
    u16 oy,
    u16 tx,
    u16 ty,
    u16 cx,
    u16 cy)
{
    if (ox == tx && oy == ty) {
        return true;
    }
    const u16 d0 = cheb(ox, oy, tx, ty);
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(ox) + k_dx[d];
        const i32 ny = static_cast<i32>(oy) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (cheb(ux, uy, tx, ty) >= d0) {
            continue;
        }
        if (can_try_step(state, unit_idx, ux, uy, cx, cy)) {
            return true;
        }
    }
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(ox) + k_dx[d];
        const i32 ny = static_cast<i32>(oy) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (cheb(ux, uy, tx, ty) > d0) {
            continue;
        }
        if (can_try_step(state, unit_idx, ux, uy, cx, cy)) {
            return true;
        }
    }
    return false;
}

static bool find_nearest_cand (
    GameState& state,
    u16 unit_idx,
    u16 player,
    u16 city_idx,
    u16 cx,
    u16 cy,
    u16 ox,
    u16 oy,
    u16* out_x,
    u16* out_y)
{
    const CircArea area = CityTileManager::work_area();
    u16 best_d = U16_KEY_NULL;
    u16 best_x = 0;
    u16 best_y = 0;
    bool found = false;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 tx = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 ty = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (tx < 0 || ty < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(tx);
        const u16 uy = static_cast<u16>(ty);
        if (!is_farm_cand(state, ux, uy, player, city_idx, cx, cy)) {
            continue;
        }
        if (!has_progress_step(state, unit_idx, ox, oy, ux, uy, cx, cy)) {
            continue;
        }
        const u16 d = cheb(ox, oy, ux, uy);
        if (!found || d < best_d) {
            found = true;
            best_d = d;
            best_x = ux;
            best_y = uy;
        }
    }
    if (!found) {
        return false;
    }
    *out_x = best_x;
    *out_y = best_y;
    return true;
}

static bool step_toward (GameState& state, u16 unit_idx, u16 tx, u16 ty, u16 cx, u16 cy) {
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (unit == nullptr) {
        return false;
    }
    const u16 d0 = cheb(unit->m_x, unit->m_y, tx, ty);
    const i32 sx = static_cast<i32>(unit->m_x) + sgn(static_cast<i32>(tx) - static_cast<i32>(unit->m_x));
    const i32 sy = static_cast<i32>(unit->m_y) + sgn(static_cast<i32>(ty) - static_cast<i32>(unit->m_y));
    if (sx >= 0 && sy >= 0 && try_step(state, unit_idx, static_cast<u16>(sx), static_cast<u16>(sy), cx, cy)) {
        return true;
    }
    u16 best_d = U16_KEY_NULL;
    i32 best_x = -1;
    i32 best_y = -1;
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        const u16 nd = cheb(ux, uy, tx, ty);
        if (nd >= d0) {
            continue;
        }
        if (!can_try_step(state, unit_idx, ux, uy, cx, cy)) {
            continue;
        }
        if (best_x < 0 || nd < best_d) {
            best_d = nd;
            best_x = nx;
            best_y = ny;
        }
    }
    if (best_x >= 0 && try_step(state, unit_idx, static_cast<u16>(best_x), static_cast<u16>(best_y), cx, cy)) {
        return true;
    }
    best_x = -1;
    best_y = -1;
    best_d = U16_KEY_NULL;
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        const u16 nd = cheb(ux, uy, tx, ty);
        if (nd > d0) {
            continue;
        }
        if (!can_try_step(state, unit_idx, ux, uy, cx, cy)) {
            continue;
        }
        if (best_x < 0 || nd < best_d) {
            best_d = nd;
            best_x = nx;
            best_y = ny;
        }
    }
    if (best_x >= 0 && try_step(state, unit_idx, static_cast<u16>(best_x), static_cast<u16>(best_y), cx, cy)) {
        return true;
    }
    return false;
}

//================================================================================================================================
//=> - DraftWorkerHandlerGeneral (mk02) -
//================================================================================================================================

bool DraftWorkerHandlerGeneral::begin (GameState& state) {
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
    g_rad = work_rad();
    g_st = &state;
    g_ok = true;
    return true;
}

void DraftWorkerHandlerGeneral::clear () {
    g_st = nullptr;
    g_ok = false;
    g_rad = 2u;
}

void DraftWorkerHandlerGeneral::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(g_ok, "DraftWorkerHandlerGeneral handle got invalid state");
    GAME_EXPECT(g_st == &state, "DraftWorkerHandlerGeneral handle got mismatched state");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "DraftWorkerHandlerGeneral handle got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "DraftWorkerHandlerGeneral handle unit has null x");
    u16 cx = 0;
    u16 cy = 0;
    u16 city_idx = 0;
    if (!home_xy(state, unit, &cx, &cy, &city_idx)) {
        return;
    }
    const u16 player = unit->m_player_idx;
    for (;;) {
        if (try_plant(state, unit->m_x, unit->m_y, player, city_idx, cx, cy)) {
            continue;
        }
        u16 tx = 0;
        u16 ty = 0;
        if (!find_nearest_cand(state, unit_idx, player, city_idx, cx, cy, unit->m_x, unit->m_y, &tx, &ty)) {
            return;
        }
        if (tx == unit->m_x && ty == unit->m_y) {
            if (!try_plant(state, tx, ty, player, city_idx, cx, cy)) {
                return;
            }
            continue;
        }
        if (!step_toward(state, unit_idx, tx, ty, cx, cy)) {
            return;
        }
        try_plant(state, unit->m_x, unit->m_y, player, city_idx, cx, cy);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
