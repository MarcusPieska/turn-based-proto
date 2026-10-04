//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "army_upgrade_mng.h"

#include "bit_array.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "general_assessor.h"
#include "log_dbg.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_roster_mng.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_upgrades.h"

//================================================================================================================================
//=> - ArmyUpgradeMng -
//================================================================================================================================

u16 ArmyUpgradeMng::upgrade_all (GameState& st, u16 seat, u16 army_hd, u16 stag_x, u16 stag_y) {
    if (st.m_statics == nullptr || st.m_player_states == nullptr || seat >= st.m_player_n) {
        return 0u;
    }
    if (army_hd == U16_KEY_NULL) {
        return 0u;
    }
    const UnitStaticData& units = st.m_statics->unit();
    PlayerState& ps = st.m_player_states[seat];
    BitArrayCL civ(st.m_statics->civ().get_item_count());
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    AssessorCtx ctx = {};
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_civ = &civ;
    ctx.m_map = &st.m_map;
    ctx.m_x = stag_x;
    ctx.m_y = stag_y;
    ctx.m_city_idx = U16_KEY_NULL;
    if (st.m_map.get_add_typ(stag_x, stag_y) == BUILD_ADD_CITY) {
        ctx.m_city_idx = st.m_map.get_add_idx(stag_x, stag_y);
    }
    u16 n_up = 0u;
    UnitAddKey cur = UnitAddKey::from_raw(army_hd);
    while (cur.is_valid()) {
        UnitAddStruct* u = st.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        const u16 from = static_cast<u16>(u->m_unit_typ_idx);
        if (from < units.get_item_count()) {
            const u16 typ = units.get_item(UnitStaticDataKey::from_raw(from)).type;
            const u16 best = UnitRosterMng::get_best_unit_of_type(typ, ctx);
            if (best != U16_KEY_NULL && best != from
                && UnitUpgrades::can_upgrade(from, best, units)
                && UnitUpgrades::can_afford(seat, from, best, units)
                && UnitUpgrades::upgrade(seat, *u, best, units)) {
                LOG_WAR_ARMY_UPGRADE::LOG(
                    static_cast<unsigned>(seat),
                    static_cast<unsigned>(ps.m_civ_index),
                    static_cast<unsigned>(from),
                    static_cast<unsigned>(best),
                    static_cast<unsigned>(st.m_current_turn));
                n_up = static_cast<u16>(n_up + 1u);
            }
        }
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return n_up;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
