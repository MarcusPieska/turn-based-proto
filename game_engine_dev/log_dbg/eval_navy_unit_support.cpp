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
#include "eval_navy_unit_support.h"

#include "city.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "trace_sink.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_type_action_map.h"

//================================================================================================================================
//= EVAL_NAVY_UNIT_SUPPORT =
//================================================================================================================================

void EVAL_NAVY_UNIT_SUPPORT::EVAL (GameState& state) {
    if (state.m_player_states == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    const u16 cn = state.m_cities.get_city_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        const PlayerState& ps = state.m_player_states[p];
        u32 local_raw = 0u;
        u32 local_rem = 0u;
        for (u16 i = 0; i < cn; ++i) {
            City* c = state.m_cities.get_city(i);
            if (c == nullptr || c->get_owner() != p) {
                continue;
            }
            local_raw += static_cast<u32>(c->calc_city_naval_unit_support(i));
            local_rem += static_cast<u32>(c->get_free_naval_unit_support());
        }
        const u32 local_used = local_raw > local_rem ? local_raw - local_rem : 0u;
        const u32 civ_free = static_cast<u32>(ps.m_free_naval_unit_support);
        const u32 unused_civ = static_cast<u32>(ps.m_target_new_naval_unit_support);
        const u32 civ_used = civ_free > unused_civ ? civ_free - unused_civ : 0u;
        const u32 paid = static_cast<u32>(ps.m_naval_unit_upkeep_needed);
        u32 units = 0u;
        for (u32 idx = 0; idx < scan_n; ++idx) {
            UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
            if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_player_idx != p) {
                continue;
            }
            const UnitStaticDataStruct& us = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx));
            if (state.m_statics->unit_type_action_map().unit_type_can_do(us.type, static_cast<u16>(UnitAction::isSeaUnit))) {
                ++units;
            }
        }
        TraceSink::printf(
            "navy support seat=%u local_free=%u local_used=%u civ_free=%u civ_used=%u paid=%u units=%u\n",
            static_cast<u32>(p), local_raw, local_used, civ_free, civ_used, paid, units);
    }
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
