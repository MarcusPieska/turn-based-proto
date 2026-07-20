//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_loop.h"
#include "assert_log.h"
#include "city.h"
#include "city_border.h"
#include "city_turn_handler.h"
#include "defensive_unit_turn_handler.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "profile_time_opt.h"
#include "research_turn_handler.h"
#include "runtime_statics.h"
#include "runtime_trace_dbg.h"
#include "settler_turn_handler.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "whiteboard_mng.h"
#include "worker_turn_handler.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u16 k_claim_cult = 25u;

static void arm_settling (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop arm_settling got nullptr player states");
    for (u16 p = 0; p < state.m_player_n; ++p) {
        state.m_player_states[p].m_target_settlements = SETTLER_MISSION_SLOTS;
    }
}

static void claim_city_borders (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        CityBorder::claim_expand(c->get_x(), c->get_y(), 0, k_claim_cult, static_cast<u8>(c->get_owner()));
    }
}

static void refill_mp (GameState& state, u16 unit_idx) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));

    GAME_EXPECT(u != nullptr, "GameLoop refill_mp got nullptr unit");
    GAME_EXPECT(state.m_statics != nullptr, "GameLoop refill_mp got nullptr statics");
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * PATH_MP_TURN);
}

static void after_city_turns (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop after_city_turns got nullptr player states");
    for (u16 p = 0; p < state.m_player_n; ++p) {
        ResearchTurnHandler::handle(state, p);
        PlayerState& ps = state.m_player_states[p];
        ps.m_last_turn_population_count = ps.m_this_turn_population_count;
        ps.m_last_turn_city_count = ps.m_this_turn_city_count;
        ps.m_this_turn_population_count = 0;
        ps.m_this_turn_city_count = 0;
        ps.m_last_turn_settler_count = 0;
        ps.m_last_turn_worker_count = 0;
        ps.m_defensive_unit_count = 0;
    }
}

static void run_city_turns (GameState& state) {
    PTO_START(PtoId::PTO_CITY_LOOP);
    SettlerTurnHandler::refresh_targets(state);
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        GAME_EXPECT(state.m_cities.get_city(i) != nullptr, "GameLoop run_city_turns got nullptr city");
        CityTurnHandler::handle(state, i);
    }
    after_city_turns(state);
    PTO_STOP(PtoId::PTO_CITY_LOOP);
}

static void run_unit_turns (GameState& state) {
    PTO_START(PtoId::PTO_UNIT_LOOP);
    GAME_EXPECT(state.m_statics != nullptr, "GameLoop run_unit_turns got nullptr statics");

    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const u16 unit_idx = static_cast<u16>(idx);
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
        
        // This check is needed because the unit vector can have gaps in the index due to recycling
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (ut == state.m_land_settler_type_idx) {
            refill_mp(state, unit_idx);
            SettlerTurnHandler::handle(state, unit_idx);
        } else if (ut == state.m_land_worker_type_idx) {
            WorkerTurnHandler::handle(state, unit_idx);
        } else if (ut == state.m_land_defense_type_idx) {
            DefensiveUnitTurnHandler::handle(state, unit_idx);
        }
    }
    PTO_STOP(PtoId::PTO_UNIT_LOOP);
}

//================================================================================================================================
//=> - GameLoop -
//================================================================================================================================

GameLoop::GameLoop () :
    m_state(nullptr) {
}

GameLoop::~GameLoop () {
    end();
}

bool GameLoop::begin (GameState* state, cstr trace_path) {
    end();
    GAME_EXPECT(state != nullptr, "GameLoop begin got nullptr state");
    GAME_EXPECT(trace_path != nullptr, "GameLoop begin got nullptr trace path");
    GAME_EXPECT(state->m_turn_limit != 0, "GameLoop begin turn limit");
    if (state == nullptr || trace_path == nullptr || state->m_turn_limit == 0) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    GAME_EXPECT(w != 0 && h != 0, "GameLoop begin map size");
    if (w == 0 || h == 0) {
        return false;
    }
    TRACE_SETUP((trace_path));
    WhiteboardMng::terminate();
    WhiteboardMng::init(w, h);
    if (!SettlerTurnHandler::begin(*state)) {
        WhiteboardMng::terminate();
        return false;
    }
    arm_settling(*state);
    ResearchTurnHandler::begin(*state);
    claim_city_borders(*state);
    m_state = state;
    return true;
}

void GameLoop::end () {
    if (m_state == nullptr) {
        return;
    }
    SettlerTurnHandler::clear();
    WhiteboardMng::terminate();
    m_state = nullptr;
}

bool GameLoop::step () {
    GAME_EXPECT(m_state != nullptr, "GameLoop step got nullptr state");
    GAME_EXPECT(m_state->m_current_turn < m_state->m_turn_limit, "GameLoop step turn limit");
    m_state->m_current_turn = m_state->m_current_turn + 1u;
    
    TRACE_NEW_TURN((static_cast<u16>(m_state->m_current_turn)));
    run_city_turns(*m_state);
    run_unit_turns(*m_state);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
