//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "game_loop.h"
#include "assert_log.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_border.h"
#include "city_tracer.h"
#include "city_turn_handler.h"
#include "combat_mng.h"
#include "defensive_unit_turn_handler.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "mock_muster_siege.h"
#include "profile_time_opt.h"
#include "research_turn_handler.h"
#include "runtime_statics.h"
#include "runtime_trace_dbg.h"
#include "settler_turn_handler.h"
#include "tile_attr_tables.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_turn_handler.h"
#include "unit_type_action_map.h"
#include "war_turn_handler.h"
#include "whiteboard_mng.h"
#include "worker_build_progress.h"
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
    u->m_mvt_points = static_cast<i16>(pts * state.m_statics->config().get_mov_pt_per_turn());
}

static void after_city_turns (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop after_city_turns got nullptr player states");
    for (u16 p = 0; p < state.m_player_n; ++p) {
        ResearchTurnHandler::handle(state, p);
        City::refresh_city_worker_flags(state, p);
        PlayerState& ps = state.m_player_states[p];
        
        ps.m_last_turn_population_count = ps.m_this_turn_population_count;
        ps.m_last_turn_city_count = ps.m_this_turn_city_count;
        ps.m_this_turn_population_count = 0;
        ps.m_this_turn_city_count = 0;
        
        ps.m_last_turn_new_land_unit_build_support = ps.m_this_turn_new_land_unit_build_support;
        ps.m_last_turn_new_naval_unit_build_support = ps.m_this_turn_new_naval_unit_build_support;
        ps.m_this_turn_new_land_unit_build_support = 0;
        ps.m_this_turn_new_naval_unit_build_support = 0;
        
        ps.m_last_turn_settler_build_n = ps.m_this_turn_settler_build_n;
        ps.m_this_turn_settler_build_n = 0;
        ps.m_last_turn_settler_count = 0;
        ps.m_last_turn_worker_count = 0;
        ps.m_defensive_unit_count = 0;
    }
}

static void run_city_turns (GameState& state) {
    PTO_START(PtoId::PTO_CITY_LOOP);
    SettlerTurnHandler::refresh_targets(state);
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop run_city_turns got nullptr player states");
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        ps.m_free_land_unit_support = 0;
        ps.m_free_naval_unit_support = 0;
        ps.m_land_unit_upkeep_needed = 0;
        ps.m_naval_unit_upkeep_needed = 0;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        GAME_EXPECT(state.m_cities.get_city(i) != nullptr, "GameLoop run_city_turns got nullptr city");
        CityTurnHandler::handle(state, i);
    }
    LOG_CITY_FLUSH(());
    after_city_turns(state);
    PTO_STOP(PtoId::PTO_CITY_LOOP);
}

static void add_upkeep (u16* dst, u16 cost) {
    const u32 sum = static_cast<u32>(*dst) + static_cast<u32>(cost);
    *dst = sum > 65535u ? 65535u : static_cast<u16>(sum);
}

static void charge_unit_upkeep (GameState& state, UnitAddStruct* u) {
    GAME_EXPECT(u != nullptr, "GameLoop charge_unit_upkeep null unit");
    GAME_EXPECT(state.m_statics != nullptr, "GameLoop charge_unit_upkeep null statics");
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop charge_unit_upkeep null player states");
    const u16 player = u->m_player_idx;
    GAME_EXPECT(player < state.m_player_n, "GameLoop charge_unit_upkeep player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    const UnitStaticDataStruct& us = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
    const bool is_land = state.m_statics->unit_type_action_map().unit_type_can_do(
        us.type, static_cast<u16>(UnitAction::isLandUnit));
    const bool is_sea = state.m_statics->unit_type_action_map().unit_type_can_do(
        us.type, static_cast<u16>(UnitAction::isSeaUnit));
    if (!is_land && !is_sea) {
        return;
    }
    City* city = nullptr;
    if (state.m_map.get_add_typ(u->m_x, u->m_y) == BUILD_ADD_CITY) {
        const u16 city_idx = state.m_map.get_add_idx(u->m_x, u->m_y);
        City* c = state.m_cities.get_city(city_idx);
        if (c != nullptr && c->get_owner() == player) {
            city = c;
        }
    }
    if (is_land) {
        if (city != nullptr) {
            city->refund_land_unit_upkeep(*u, &ps);
        } else {
            add_upkeep(&ps.m_land_unit_upkeep_needed, 1u);
        }
        return;
    }
    if (city != nullptr) {
        city->refund_naval_unit_upkeep(*u, &ps);
    } else {
        add_upkeep(&ps.m_naval_unit_upkeep_needed, 1u);
    }
}

static void settle_unit_upkeep (GameState& state) {
    GAME_EXPECT(state.m_player_states != nullptr, "GameLoop settle_unit_upkeep null player states");
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_land_unit_upkeep_needed > ps.m_free_land_unit_support) {
            ps.m_target_new_land_unit_support = 0;
            ps.m_land_unit_upkeep_needed = static_cast<u16>(ps.m_land_unit_upkeep_needed - ps.m_free_land_unit_support);
        } else {
            ps.m_target_new_land_unit_support = static_cast<u16>(ps.m_free_land_unit_support - ps.m_land_unit_upkeep_needed);
            ps.m_land_unit_upkeep_needed = 0;
        }
        if (ps.m_naval_unit_upkeep_needed > ps.m_free_naval_unit_support) {
            ps.m_target_new_naval_unit_support = 0;
            ps.m_naval_unit_upkeep_needed = static_cast<u16>(ps.m_naval_unit_upkeep_needed - ps.m_free_naval_unit_support);
        } else {
            ps.m_target_new_naval_unit_support = static_cast<u16>(ps.m_free_naval_unit_support - ps.m_naval_unit_upkeep_needed);
            ps.m_naval_unit_upkeep_needed = 0;
        }
        const u32 cost = static_cast<u32>(ps.m_land_unit_upkeep_needed) + static_cast<u32>(ps.m_naval_unit_upkeep_needed);
        if (cost >= ps.m_commerce) {
            ps.m_commerce = 0;
        } else {
            ps.m_commerce = ps.m_commerce - cost;
        }
    }
}

static bool seat_cap_xy (const GameState& state, u16 seat, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr || seat >= state.m_player_n) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        *ox = c->get_x();
        *oy = c->get_y();
        return true;
    }
    return false;
}

static bool pick_near_non_lucky (const GameState& state, u16 seat, u16* out_enemy) {
    if (out_enemy == nullptr || state.m_player_states == nullptr) {
        return false;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!seat_cap_xy(state, seat, &sx, &sy)) {
        return false;
    }
    u32 best_d = 0xFFFFFFFFu;
    u16 best = U16_KEY_NULL;
    for (u16 e = 0; e < state.m_player_n; ++e) {
        if (e == seat || state.m_player_states[e].m_lucky != 0u) {
            continue;
        }
        if (state.m_player_states[e].m_is_active == 0u) {
            continue;
        }
        u16 ex = 0u;
        u16 ey = 0u;
        if (!seat_cap_xy(state, e, &ex, &ey)) {
            continue;
        }
        const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
        const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
        const u32 d = adx + ady;
        if (d < best_d) {
            best_d = d;
            best = e;
        }
    }
    if (best == U16_KEY_NULL) {
        return false;
    }
    *out_enemy = best;
    return true;
}

static void check_start_wars (GameState& state) {
    if (state.m_player_states == nullptr || state.m_player_n == 0u) {
        return;
    }
    static const u16 k_lucky_cap = 256u;
    u16 lucky[k_lucky_cap];
    u16 n_lucky = 0u;
    for (u16 p = 0; p < state.m_player_n; ++p) {
        const PlayerState& ps = state.m_player_states[p];
        if (ps.m_lucky == 0u || ps.m_is_active == 0u) {
            continue;
        }
        if (n_lucky >= k_lucky_cap) {
            break;
        }
        lucky[n_lucky++] = p;
    }
    if (n_lucky == 0u) {
        return;
    }
    const u32 turn = state.m_current_turn;
    if (turn == 0u) {
        return;
    }
    const u16 li = static_cast<u16>(turn % static_cast<u32>(n_lucky));
    const u16 seat = lucky[li];
    PlayerState& ps = state.m_player_states[seat];
    if (ps.m_at_war != 0u || WarTurnHandler::is_engaged(seat)) {
        return;
    }
    u16 enemy = U16_KEY_NULL;
    if (!WarTurnHandler::pick_enemy(state, seat, &enemy)) {
        if (!pick_near_non_lucky(state, seat, &enemy)) {
            return;
        }
    }
    MockMusterSiege mock;
    if (!mock.collect(state, seat)) {
        return;
    }
    u16 taken = 0u;
    u16 foe_n = 0u;
    u16 war_turns = 0u;
    if (!mock.campaign(state, enemy, &taken, &foe_n, &war_turns) || taken == 0u) {
        return;
    }
    if (!WarTurnHandler::engage(state, seat, enemy)) {
        return;
    }
    ps.m_at_war = 1u;
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
        charge_unit_upkeep(state, u);
        const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (ut == state.m_land_settler_type_idx) {
            refill_mp(state, unit_idx);
            SettlerTurnHandler::handle(state, unit_idx);
        } else if (ut == state.m_land_worker_type_idx) {
            WorkerBuildProgress::refill_mp(state, unit_idx);
            WorkerTurnHandler::handle(state, unit_idx);
        } else if (ut == state.m_land_defense_type_idx) {
            refill_mp(state, unit_idx);
            DefensiveUnitTurnHandler::handle(state, unit_idx);
        } else {
            refill_mp(state, unit_idx);
        }
        UnitTurnHandler::handle(state, unit_idx);
    }
    settle_unit_upkeep(state);
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
    if (WhiteboardMng::width() != w || WhiteboardMng::height() != h) {
        if (WhiteboardMng::chkout() != 0u) {
            return false;
        }
        WhiteboardMng::terminate();
        WhiteboardMng::init(w, h);
    }
    if (state->m_statics == nullptr) {
        return false;
    }
    if (!TileAttrTables::ready() && !TileAttrTables::setup(*state->m_statics)) {
        return false;
    }
    if (!UnitMovementMng::setup_mvt_costs(*state->m_statics)) {
        return false;
    }
    if (!CombatMng::setup(*state->m_statics)) {
        return false;
    }
    if (!SettlerTurnHandler::begin(*state)) {
        CombatMng::clear();
        WhiteboardMng::terminate();
        return false;
    }
    if (!WarTurnHandler::begin(*state)) {
        SettlerTurnHandler::clear();
        CombatMng::clear();
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
    WarTurnHandler::clear();
    SettlerTurnHandler::clear();
    CombatMng::clear();
    if (WhiteboardMng::chkout() == 0u) {
        WhiteboardMng::terminate();
    }
    m_state = nullptr;
}

bool GameLoop::step () {
    GAME_EXPECT(m_state != nullptr, "GameLoop step got nullptr state");
    GAME_EXPECT(m_state->m_current_turn < m_state->m_turn_limit, "GameLoop step turn limit");
    m_state->m_current_turn = m_state->m_current_turn + 1u;
    
    TRACE_NEW_TURN((static_cast<u16>(m_state->m_current_turn)));
    check_start_wars(*m_state);
    run_city_turns(*m_state);
    run_unit_turns(*m_state);
    WarTurnHandler::handle(*m_state);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
