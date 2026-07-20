//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "worker_turn_handler.h"
#include "assert_log.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_worker_typ (const GameState& state, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = state.m_statics->unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(state.m_statics->unit_type().get_name(tk), "LAND_WORKER") == 0;
}

//================================================================================================================================
//=> - WorkerTurnHandler -
//================================================================================================================================

void WorkerTurnHandler::handle (GameState& state, u16 unit_idx) {
    GAME_EXPECT(state.m_player_states != nullptr, "WorkerTurnHandler got nullptr player states");
    GAME_EXPECT(state.m_statics != nullptr, "WorkerTurnHandler got nullptr statics");
    UnitAddStruct* unit = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    
    GAME_EXPECT(unit != nullptr, "WorkerTurnHandler got nullptr unit");
    GAME_EXPECT(unit->m_x != U16_KEY_NULL, "WorkerTurnHandler unit has null x");
    if (!is_worker_typ(state, unit->m_unit_typ_idx)) {
        return;
    }
    const u16 player = unit->m_player_idx;

    GAME_EXPECT(player < state.m_player_n, "WorkerTurnHandler player out of bounds");
    PlayerState& ps = state.m_player_states[player];
    ps.m_last_turn_worker_count = static_cast<u16>(ps.m_last_turn_worker_count + 1u);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
