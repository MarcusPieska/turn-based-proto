//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "anchored_farmer.h"

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

#include "anchored_farmer_path.inc"

//================================================================================================================================
//=> - State -
//================================================================================================================================

static bool g_ok = false;
static GameState* g_st = nullptr;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 cheb (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
    const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool in_bounds (const GameState& state, u16 x, u16 y) {
    return x < state.m_map.width() && y < state.m_map.height();
}

static bool map_at (i32 ox, i32 oy, u16* out_i) {
    if (ox < -k_map_r || ox > k_map_r || oy < -k_map_r || oy > k_map_r) {
        return false;
    }
    *out_i = static_cast<u16>((ox + k_map_r) + (oy + k_map_r) * static_cast<i32>(k_map_side));
    return true;
}

static bool find_anchor (const GameState& state, u16 player, u16 x, u16 y, u16* ax, u16* ay) {
    if (ax == nullptr || ay == nullptr) {
        return false;
    }
    const CircArea area = CityTileManager::work_area();
    if (area.m_lim == 0) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u16 best_d = U16_KEY_NULL;
    u16 bx = 0;
    u16 by = 0;
    bool hit = false;
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 tx = static_cast<i32>(x) + static_cast<i32>(area.m_brd[i][0]);
        const i32 ty = static_cast<i32>(y) + static_cast<i32>(area.m_brd[i][1]);
        if (tx < 0 || ty < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(tx);
        const u16 uy = static_cast<u16>(ty);
        if (ux >= w || uy >= h) {
            continue;
        }
        if (state.m_map.get_add_typ(ux, uy) != BUILD_ADD_CITY) {
            continue;
        }
        const u16 city_idx = state.m_map.get_add_idx(ux, uy);
        const City* c = state.m_cities.get_city(city_idx);
        if (c == nullptr || c->get_owner() != player) {
            continue;
        }
        if (c->get_x() != ux || c->get_y() != uy) {
            continue;
        }
        const u16 d = cheb(x, y, ux, uy);
        if (!hit || d < best_d) {
            best_d = d;
            bx = ux;
            by = uy;
            hit = true;
        }
    }
    if (!hit) {
        return false;
    }
    *ax = bx;
    *ay = by;
    return true;
}

static bool is_farmable (const GameState& state, u16 x, u16 y, u16 player) {
    if (!in_bounds(state, x, y)) {
        return false;
    }
    if (state.m_map.get_terrain(x, y) != TERR_PLAINS[0]) {
        return false;
    }
    if (state.m_map.get_add_typ(x, y) != BUILD_ADD_STD) {
        return false;
    }
    const u16 add_idx = state.m_map.get_add_idx(x, y);
    if (add_idx != U16_KEY_NULL) {
        if (StdAddHelper::has_farm(state.m_map.tile(x, y))) {
            return false;
        }
    }
    if (state.m_map.get_civ_owner(x, y) != static_cast<u8>(player)) {
        return false;
    }
    return true;
}

static bool try_plant (GameState& state, u16 x, u16 y, u16 player) {
    if (!is_farmable(state, x, y, player)) {
        return false;
    }
    if (!state.m_map.set_tile_add(x, y, 0u, BUILD_ADD_STD)) {
        return false;
    }
    StdAddHelper::set_farm(state.m_map.tile(x, y));
    return true;
}

static bool try_step (GameState& state, u16 unit_idx, u16 ux, u16 uy) {
    const UnitAddKey key = UnitAddKey::from_raw(unit_idx);
    i16 cost = 0;
    if (!UnitMovementMng::can_step(state, key, ux, uy, &cost)) {
        return false;
    }
    return UnitMovementMng::apply_step(state, key, ux, uy);
}

static bool step_from_map (GameState& state, u16 unit_idx, i32 ox, i32 oy) {
    u16 mi = 0;
    i32 sdx = 0;
    i32 sdy = 0;
    if (map_at(ox, oy, &mi) && (k_step_x[mi] != 0 || k_step_y[mi] != 0)) {
        sdx = k_step_x[mi];
        sdy = k_step_y[mi];
    } else {
        sdx = -sgn(ox);
        sdy = -sgn(oy);
        if (sdx == 0 && sdy == 0) {
            return false;
        }
    }
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (unit == nullptr) {
        return false;
    }
    const i32 nx = static_cast<i32>(unit->m_x) + sdx;
    const i32 ny = static_cast<i32>(unit->m_y) + sdy;
    if (nx < 0 || ny < 0) {
        return false;
    }
    return try_step(state, unit_idx, static_cast<u16>(nx), static_cast<u16>(ny));
}

//================================================================================================================================
//=> - AnchoredFarmer -
//================================================================================================================================

bool AnchoredFarmer::begin (GameState& state) {
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
    g_st = &state;
    g_ok = true;
    return true;
}

void AnchoredFarmer::clear () {
    g_st = nullptr;
    g_ok = false;
}

void AnchoredFarmer::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(g_ok, "AnchoredFarmer handle got invalid state");
    GAME_EXPECT(g_st == &state, "AnchoredFarmer handle got mismatched state");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "AnchoredFarmer handle got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "AnchoredFarmer handle unit has null x");
    const u16 player = unit->m_player_idx;
    for (;;) {
        u16 ax = 0;
        u16 ay = 0;
        if (!find_anchor(state, player, unit->m_x, unit->m_y, &ax, &ay)) {
            return;
        }
        try_plant(state, unit->m_x, unit->m_y, player);
        const i32 ox = static_cast<i32>(unit->m_x) - static_cast<i32>(ax);
        const i32 oy = static_cast<i32>(unit->m_y) - static_cast<i32>(ay);
        if (!step_from_map(state, unit_idx, ox, oy)) {
            return;
        }
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
