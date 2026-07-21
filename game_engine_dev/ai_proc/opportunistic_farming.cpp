//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "opportunistic_farming.h"

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

//================================================================================================================================
//=> - State -
//================================================================================================================================

static bool g_ok = false;
static GameState* g_st = nullptr;

static const i32 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i32 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
static const u8 k_dir_n = 8u;
static const u8 k_riv_scan = 4u;
static const u16 k_bfs_cap = 48u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool in_bounds (const GameState& state, u16 x, u16 y) {
    return x < state.m_map.width() && y < state.m_map.height();
}

static bool is_river (const GameState& state, u16 x, u16 y) {
    return in_bounds(state, x, y) && state.m_map.get_river(x, y) != 0;
}

static bool in_work_reach (const GameState& state, u16 player, u16 x, u16 y) {
    const CircArea area = CityTileManager::work_area();
    if (area.m_lim == 0) {
        return false;
    }
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
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
        if (c->get_x() == ux && c->get_y() == uy) {
            return true;
        }
    }
    return false;
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
    if (!in_work_reach(state, player, x, y)) {
        return false;
    }
    return true;
}

static bool is_riv_adj_plain (const GameState& state, u16 x, u16 y, u16 player) {
    if (!is_farmable(state, x, y, player) || is_river(state, x, y)) {
        return false;
    }
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(x) + k_dx[d];
        const i32 ny = static_cast<i32>(y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        if (is_river(state, static_cast<u16>(nx), static_cast<u16>(ny))) {
            return true;
        }
    }
    return false;
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

static bool bfs_seen (const u16* sx, const u16* sy, u16 n, u16 x, u16 y) {
    for (u16 i = 0; i < n; ++i) {
        if (sx[i] == x && sy[i] == y) {
            return true;
        }
    }
    return false;
}

static bool riv_next_step (
    GameState& state,
    u16 ox,
    u16 oy,
    u16 player,
    u8 rem,
    u16* out_x,
    u16* out_y)
{
    if (rem == 0 || out_x == nullptr || out_y == nullptr) {
        return false;
    }
    u16 sx[k_bfs_cap];
    u16 sy[k_bfs_cap];
    u8 prev[k_bfs_cap];
    u8 len[k_bfs_cap];
    u16 n = 0;
    sx[0] = ox;
    sy[0] = oy;
    prev[0] = 255u;
    len[0] = 0;
    n = 1;
    u16 qi = 0;
    u16 hit = U16_KEY_NULL;
    while (qi < n) {
        const u16 cx = sx[qi];
        const u16 cy = sy[qi];
        if (len[qi] > 0 && is_river(state, cx, cy) && is_farmable(state, cx, cy, player)) {
            hit = qi;
            break;
        }
        if (len[qi] >= rem) {
            qi = static_cast<u16>(qi + 1u);
            continue;
        }
        for (u8 d = 0; d < k_dir_n; ++d) {
            const i32 nx = static_cast<i32>(cx) + k_dx[d];
            const i32 ny = static_cast<i32>(cy) + k_dy[d];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (!is_river(state, ux, uy) || bfs_seen(sx, sy, n, ux, uy)) {
                continue;
            }
            if (n >= k_bfs_cap) {
                break;
            }
            sx[n] = ux;
            sy[n] = uy;
            prev[n] = static_cast<u8>(qi);
            len[n] = static_cast<u8>(len[qi] + 1u);
            n = static_cast<u16>(n + 1u);
        }
        qi = static_cast<u16>(qi + 1u);
    }
    if (hit == U16_KEY_NULL) {
        return false;
    }
    u16 at = hit;
    while (prev[at] != 0) {
        at = prev[at];
    }
    *out_x = sx[at];
    *out_y = sy[at];
    return true;
}

static bool step_farmable_riv (GameState& state, UnitAddStruct* unit, u16 unit_idx) {
    const u16 player = unit->m_player_idx;
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_river(state, ux, uy) || !is_farmable(state, ux, uy, player)) {
            continue;
        }
        if (try_step(state, unit_idx, ux, uy)) {
            return true;
        }
    }
    return false;
}

static bool step_riv_adj_plain (GameState& state, UnitAddStruct* unit, u16 unit_idx) {
    const u16 player = unit->m_player_idx;
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_riv_adj_plain(state, ux, uy, player)) {
            continue;
        }
        if (try_step(state, unit_idx, ux, uy)) {
            return true;
        }
    }
    return false;
}

static bool step_any_farmable (GameState& state, UnitAddStruct* unit, u16 unit_idx) {
    const u16 player = unit->m_player_idx;
    for (u8 d = 0; d < k_dir_n; ++d) {
        const i32 nx = static_cast<i32>(unit->m_x) + k_dx[d];
        const i32 ny = static_cast<i32>(unit->m_y) + k_dy[d];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(nx);
        const u16 uy = static_cast<u16>(ny);
        if (!is_farmable(state, ux, uy, player)) {
            continue;
        }
        if (try_step(state, unit_idx, ux, uy)) {
            return true;
        }
    }
    return false;
}

static i32 sgn (i32 v) {
    return (v > 0) - (v < 0);
}

static bool step_cheb_ring (GameState& state, UnitAddStruct* unit, u16 unit_idx, i32 ring) {
    const u16 player = unit->m_player_idx;
    const i32 ox = static_cast<i32>(unit->m_x);
    const i32 oy = static_cast<i32>(unit->m_y);
    for (i32 dy = -ring; dy <= ring; ++dy) {
        for (i32 dx = -ring; dx <= ring; ++dx) {
            const i32 adx = dx < 0 ? -dx : dx;
            const i32 ady = dy < 0 ? -dy : dy;
            const i32 cheb = adx > ady ? adx : ady;
            if (cheb != ring) {
                continue;
            }
            const i32 tx = ox + dx;
            const i32 ty = oy + dy;
            if (tx < 0 || ty < 0) {
                continue;
            }
            const u16 ux = static_cast<u16>(tx);
            const u16 uy = static_cast<u16>(ty);
            if (!is_farmable(state, ux, uy, player)) {
                continue;
            }
            const i32 sx = ox + sgn(dx);
            const i32 sy = oy + sgn(dy);
            if (sx < 0 || sy < 0) {
                continue;
            }
            if (try_step(state, unit_idx, static_cast<u16>(sx), static_cast<u16>(sy))) {
                return true;
            }
        }
    }
    return false;
}

static bool step_cheb_expand (GameState& state, UnitAddStruct* unit, u16 unit_idx) {
    return step_cheb_ring(state, unit, unit_idx, 2)
        || step_cheb_ring(state, unit, unit_idx, 3)
        || step_cheb_ring(state, unit, unit_idx, 4);
}

static u8 river_cruise (GameState& state, UnitAddStruct* unit, u16 unit_idx, u8 budget) {
    const u16 player = unit->m_player_idx;
    u8 used = 0;
    while (used < budget) {
        if (step_farmable_riv(state, unit, unit_idx)) {
            try_plant(state, unit->m_x, unit->m_y, player);
            used = static_cast<u8>(used + 1u);
            continue;
        }
        u16 nx = 0;
        u16 ny = 0;
        const u8 rem = static_cast<u8>(budget - used);
        if (!riv_next_step(state, unit->m_x, unit->m_y, player, rem, &nx, &ny)) {
            break;
        }
        if (!try_step(state, unit_idx, nx, ny)) {
            break;
        }
        try_plant(state, unit->m_x, unit->m_y, player);
        used = static_cast<u8>(used + 1u);
    }
    return used;
}

//================================================================================================================================
//=> - OpportunisticFarming -
//================================================================================================================================

bool OpportunisticFarming::begin (GameState& state) {
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

void OpportunisticFarming::clear () {
    g_st = nullptr;
    g_ok = false;
}

void OpportunisticFarming::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(g_ok, "OpportunisticFarming handle got invalid state");
    GAME_EXPECT(g_st == &state, "OpportunisticFarming handle got mismatched state");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    GAME_EXPECT(unit != nullptr, "OpportunisticFarming handle got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "OpportunisticFarming handle unit has null x");
    const u16 player = unit->m_player_idx;
    try_plant(state, unit->m_x, unit->m_y, player);
    if (is_river(state, unit->m_x, unit->m_y)) {
        river_cruise(state, unit, unit_idx, k_riv_scan);
        if (!is_farmable(state, unit->m_x, unit->m_y, player)) {
            if (step_riv_adj_plain(state, unit, unit_idx)) {
                try_plant(state, unit->m_x, unit->m_y, player);
            } else if (step_any_farmable(state, unit, unit_idx)) {
                try_plant(state, unit->m_x, unit->m_y, player);
            } else if (step_cheb_expand(state, unit, unit_idx)) {
                try_plant(state, unit->m_x, unit->m_y, player);
            }
        }
        return;
    }
    if (step_farmable_riv(state, unit, unit_idx)) {
        try_plant(state, unit->m_x, unit->m_y, player);
        river_cruise(state, unit, unit_idx, static_cast<u8>(k_riv_scan - 1u));
        return;
    }
    if (step_riv_adj_plain(state, unit, unit_idx)) {
        try_plant(state, unit->m_x, unit->m_y, player);
        return;
    }
    if (step_any_farmable(state, unit, unit_idx)) {
        try_plant(state, unit->m_x, unit->m_y, player);
        return;
    }
    if (step_cheb_expand(state, unit, unit_idx)) {
        try_plant(state, unit->m_x, unit->m_y, player);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
