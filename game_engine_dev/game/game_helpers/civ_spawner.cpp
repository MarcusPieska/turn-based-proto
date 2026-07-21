//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "civ_spawner.h"

#include <cstring>

#include "build_adds_array.h"
#include "config_settings_static.h"
#include "city.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "runtime_trace_dbg.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - CivSpawner -
//================================================================================================================================

static bool is_settler_typ (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = st.unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(st.unit_type().get_name(tk), "LAND_SETTLER") == 0;
}

static bool found_city (GameState* state, u16 x, u16 y, u16 civ_idx) {
    if (state->m_map.get_add_idx(x, y) != U16_KEY_NULL) {
        return false;
    }
    const u16 city_idx = state->m_cities.get_next_new_city_idx();
    City* city = state->m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(civ_idx, x, y);
    if (!state->m_map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    TRACE_CITY_FOUNDATION((x, y, civ_idx));
    return true;
}

bool CivSpawner::spawn (GameState* state, u16 x, u16 y, u16 civ_idx) {
    if (state == nullptr || state->m_statics == nullptr) {
        return false;
    }
    if (state->m_player_n == 0 || civ_idx >= state->m_player_n) {
        return false;
    }
    const ConfigListUnit& start_units = state->m_statics->config().get_start_units();
    if (start_units.n == 0) {
        return false;
    }
    TRACE_CIV_SPAWN_PT((x, y, civ_idx));
    bool city_done = false;
    for (u16 k = 0; k < start_units.n; ++k) {
        const u16 typ_idx = start_units.keys[k].value();
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(*state, x, y, civ_idx, typ_idx, &key)) {
            return false;
        }
        TRACE_UNIT_SPAWN((typ_idx, civ_idx, x, y));
        if (!city_done && is_settler_typ(*state->m_statics, typ_idx)) {
            if (!found_city(state, x, y, civ_idx)) {
                return false;
            }
            city_done = true;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
