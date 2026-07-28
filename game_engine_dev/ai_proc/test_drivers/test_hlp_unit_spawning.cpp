//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_unit_spawning.h"

#include <cstring>

#include "city.h"
#include "city_array.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 find_unit_by_name (const RuntimeStatics& st, cstr name) {
    if (name == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - TestHlpUnitSpawning -
//================================================================================================================================

bool TestHlpUnitSpawning::spawn_at_cities (GameState& s, u16 player, cstr name, u16 count) {
    if (s.m_statics == nullptr || player >= s.m_player_n || name == nullptr) {
        return false;
    }
    if (count == 0u) {
        return true;
    }
    if (count > 64u) {
        return false;
    }
    const u16 unit_idx = find_unit_by_name(*s.m_statics, name);
    if (unit_idx == U16_KEY_NULL) {
        return false;
    }
    u16 typs[64];
    for (u16 i = 0; i < count; ++i) {
        typs[i] = unit_idx;
    }
    const u16 cn = s.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != player) {
            continue;
        }
        if (!s.spawn(c->get_x(), c->get_y(), player, typs, count)) {
            return false;
        }
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
