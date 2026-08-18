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

static bool home_xy (const GameState& state, const UnitAddStruct* unit, u16* ox, u16* oy) {
    const u16 city_idx = WorkerHelper::get_data(unit);
    const City* c = state.m_cities.get_city(city_idx);
    if (c == nullptr || c->get_owner() != unit->m_player_idx) {
        return false;
    }
    *ox = c->get_x();
    *oy = c->get_y();
    return true;
}

static bool in_disk (const GameState& state, const UnitAddStruct* unit, u16 x, u16 y) {
    u16 cx = 0;
    u16 cy = 0;
    if (!home_xy(state, unit, &cx, &cy)) {
        return false;
    }
    return cheb(x, y, cx, cy) <= g_rad;
}

static bool is_river (const GameState& state, u16 x, u16 y) {
    return in_bounds(state, x, y) && state.m_map.get_river(x, y) != 0;
}

static bool is_farmable (const GameState& state, const UnitAddStruct* unit, u16 x, u16 y) {
    if (!in_bounds(state, x, y) || !in_disk(state, unit, x, y)) {
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
    if (state.m_map.get_civ_owner(x, y) != static_cast<u8>(unit->m_player_idx)) {
        return false;
    }
    return true;
}

static bool try_plant (GameState& state, const UnitAddStruct* unit, u16 x, u16 y) {
    if (!is_farmable(state, unit, x, y)) {
        return false;
    }
    if (!state.m_map.set_tile_add(x, y, 0u, BUILD_ADD_STD)) {
        return false;
    }
    StdAddHelper::set_farm(state.m_map.tile(x, y));
    return true;
}

static bool try_step (GameState& state, UnitAddStruct* unit, u16 unit_idx, u16 ux, u16 uy) {
    if (!in_disk(state, unit, ux, uy)) {
        return false;
    }
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    if (!UnitMovementMng::can_step(state, key, ux, uy, &cost)) {
        return false;
    }
    return UnitMovementMng::apply_step(state, key, ux, uy);
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool step_pref (GameState& state, UnitAddStruct* unit, u16 unit_idx, bool want_riv) {
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_farmable(state, unit, ux, uy)) {
            continue;
        }
        if (want_riv && !is_river(state, ux, uy)) {
            continue;
        }
        if (try_step(state, unit, unit_idx, ux, uy)) {
            return true;
        }
    }
    return false;
}

static bool step_ring (GameState& state, UnitAddStruct* unit, u16 unit_idx, i32 ring) {
    const i32 ox = static_cast<i32>(unit->m_x);
    const i32 oy = static_cast<i32>(unit->m_y);
    for (i32 dy = -ring; dy <= ring; ++dy) {
        for (i32 dx = -ring; dx <= ring; ++dx) {
            const i32 adx = dx < 0 ? -dx : dx;
            const i32 ady = dy < 0 ? -dy : dy;
            const i32 c = adx > ady ? adx : ady;
            if (c != ring) {
                continue;
            }
            const i32 tx = ox + dx;
            const i32 ty = oy + dy;
            if (tx < 0 || ty < 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(tx);
            const u16 uy = static_cast<u16>(ty);
            if (!is_farmable(state, unit, ux, uy)) {
                continue;
            }
            const i32 sx = ox + sgn(dx);
            const i32 sy = oy + sgn(dy);
            if (sx < 0 || sy < 0) {
                continue;
            }
            if (try_step(state, unit, unit_idx, static_cast<u16>(sx), static_cast<u16>(sy))) {
                return true;
            }
        }
    }
    return false;
}

static bool step_expand (GameState& state, UnitAddStruct* unit, u16 unit_idx) {
    const i32 lim = static_cast<i32>(g_rad);
    for (i32 r = 2; r <= lim; ++r) {
        if (step_ring(state, unit, unit_idx, r)) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - DraftWorkerHandlerGeneral (mk01) -
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
    try_plant(state, unit, unit->m_x, unit->m_y);
    if (step_pref(state, unit, unit_idx, true)) {
        try_plant(state, unit, unit->m_x, unit->m_y);
        return;
    }
    if (step_pref(state, unit, unit_idx, false)) {
        try_plant(state, unit, unit->m_x, unit->m_y);
        return;
    }
    if (step_expand(state, unit, unit_idx)) {
        try_plant(state, unit, unit->m_x, unit->m_y);
    } 
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
