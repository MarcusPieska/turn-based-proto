//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "game_loop.h"
#include "city.h"
#include "city_turn_handler.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "runtime_trace_dbg.h"
#include "settler_turn_mng.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 find_settler_typ (const RuntimeStatics& st) {
    const u16 n = st.unit().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(st.unit().get_name(UnitStaticDataKey::from_raw(i)), "Settler") == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool is_settler_typ (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = st.unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(st.unit_type().get_name(tk), "LAND_SETTLER") == 0;
}

static void refill_mp (GameState& state, u16 unit_idx) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * PATH_MP_TURN);
}

static void run_city_turns (GameState& state) {
    SettlerTurnMng::refresh_targets(state);
    const u16 settler_typ = (state.m_statics != nullptr) ? find_settler_typ(*state.m_statics) : U16_KEY_NULL;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* city = state.m_cities.get_city(i);
        if (city == nullptr) {
            continue;
        }
        const u16 player = city->get_owner();
        if (SettlerTurnMng::need_settler(state, player) && settler_typ != U16_KEY_NULL) {
            city->build_unit(settler_typ);
            city->add_production(i, 60000u);
        }
        CityTurnHandler::handle(state, i);
    }
}

static void run_unit_turns (GameState& state) {
    SettlerTurnMng::begin_unit_pass(state);
    const u32 scan_n = static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const u16 unit_idx = static_cast<u16>(idx);
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
        if (u == nullptr || u->m_x == U16_KEY_NULL || state.m_statics == nullptr) {
            continue;
        }
        if (!is_settler_typ(*state.m_statics, u->m_unit_typ_idx)) {
            continue;
        }
        refill_mp(state, unit_idx);
        SettlerTurnMng::handle(state, unit_idx);
    }
}

//================================================================================================================================
//=> - GameLoop -
//================================================================================================================================

GameLoop::GameLoop () :
    m_state(nullptr) {
}

GameLoop::~GameLoop () {
}

bool GameLoop::begin (GameState* state, cstr trace_path) {
    m_state = nullptr;
    if (state == nullptr || trace_path == nullptr) {
        return false;
    }
    if (state->m_turn_limit == 0) {
        return false;
    }
    TRACE_SETUP((trace_path));
    m_state = state;
    return true;
}

bool GameLoop::step () {
    if (m_state == nullptr) {
        return false;
    }
    if (m_state->m_current_turn >= m_state->m_turn_limit) {
        return false;
    }
    m_state->m_current_turn = m_state->m_current_turn + 1u;
    TRACE_NEW_TURN((static_cast<u16>(m_state->m_current_turn)));
    run_city_turns(*m_state);
    run_unit_turns(*m_state);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
