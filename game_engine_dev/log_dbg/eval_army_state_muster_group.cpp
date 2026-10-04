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
#include "eval_army_state_muster_group.h"

#include "game_state.h"
#include "log_army_info.h"
#include "log_unit_state.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"

#include <cstdio>

//================================================================================================================================
//= EVAL_ARMY_STATE_MUSTER_GROUP =
//================================================================================================================================

void EVAL_ARMY_STATE_MUSTER_GROUP::EVAL (GameState& state, u16 sx, u16 sy, const u16* heads, u16 n) {
    if (heads == nullptr || n == 0u) {
        return;
    }
    char stg[48];
    if (std::snprintf(stg, sizeof(stg), "musterStg_%u_%u", static_cast<unsigned>(sx), static_cast<unsigned>(sy)) < 0) {
        return;
    }
    LOG_ARMY_INFO::LOG(stg);
    for (u16 i = 0; i < n; ++i) {
        if (heads[i] == U16_KEY_NULL) {
            continue;
        }
        UnitAddKey cur = UnitAddKey::from_raw(heads[i]);
        const UnitAddStruct* hu = state.m_units.get_unit_add(cur);
        if (hu == nullptr) {
            continue;
        }
        u16 gx = hu->m_x;
        u16 gy = hu->m_y;
        LOG_ARMY_INFO::LOG("musterGroup");
        while (cur.is_valid()) {
            const UnitAddStruct* u = state.m_units.get_unit_add(cur);
            if (u == nullptr) {
                break;
            }
            u16 x = u->m_x;
            u16 y = u->m_y;
            if (x == U16_KEY_NULL) {
                x = gx;
                y = gy;
            }
            LOG_UNIT_STATE::LOG(u->m_player_idx, cur.value(), x, y);
            if (u->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
        }
        LOG_ARMY_INFO::LOG("end");
    }
}

//================================================================================================================================
//= End of file =
//================================================================================================================================
