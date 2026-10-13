//================================================================================================================================
//= WARNING =
//================================================================================================================================
//
//  - First-create stub from gen_log_dbg.py (TEMPLATE_eval.cpp).
//  - Regen will NOT overwrite this file once it exists; implement EVAL here.
//  - Real body always compiled into log_dbg.so (LOG_DBG_SO_BUILD).
//
//================================================================================================================================
//= Includes =
//================================================================================================================================

#define LOG_DBG_SO_BUILD
#include "eval_count_artillery.h"

#include "game_state.h"
#include "log_unit_type_count.h"
#include "runtime_statics.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//= EVAL_COUNT_ARTILLERY =
//================================================================================================================================

void EVAL_COUNT_ARTILLERY::EVAL (GameState& state, u16 player) {
    if (state.m_statics == nullptr || player >= state.m_player_n) {
        return;
    }
    const UnitTypeActionMap& am = state.m_statics->unit_type_action_map();
    const u16 act_br = static_cast<u16>(UnitAction::canBarrage);
    const u16 typ_n = state.m_statics->unit().get_item_count();
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    u16 arty = 0u;
    u16 tot = 0u;
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_player_idx != player) {
            continue;
        }
        tot = static_cast<u16>(tot + 1u);
        if (u->m_unit_typ_idx >= typ_n) {
            continue;
        }
        const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (ut != U16_KEY_NULL && am.unit_type_can_do(ut, act_br)) {
            arty = static_cast<u16>(arty + 1u);
        }
    }
    LOG_UNIT_TYPE_COUNT::LOG(player, "artillery", arty, tot);
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
