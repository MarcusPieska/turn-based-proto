//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "war_target_policy.h"

#include "game_state.h"
#include "log_dbg.h"
#include "war_target_policy_sector_flood.h"
#include "war_target_secure_capital.h"

//================================================================================================================================
//=> - State -
//================================================================================================================================

static WarTargetKind g_kind = WarTargetKind::SecureCapital;

//================================================================================================================================
//=> - War target entry -
//================================================================================================================================

void war_target_set_kind (WarTargetKind k) {
    g_kind = k;
}

WarTargetKind war_target_get_kind () {
    return g_kind;
}

bool war_target_pick (GameState& st, u16 seat, u16 from_x, u16 from_y, WarTargetPlan* io) {
    bool ok = false;
    switch (g_kind) {
    case WarTargetKind::SectorFlood:
        ok = war_target_pick_sector_flood(st, seat, from_x, from_y, io);
        break;
    case WarTargetKind::SecureCapital:
    default:
        ok = war_target_secure_capital(st, seat, from_x, from_y, io);
        break;
    }
    if (ok && io != nullptr) {
        LOG_WAR_TARGETS::LOG(SC_U32(seat), SC_U32(io->m_enemy), SC_U32(io->m_n), SC_U32(st.m_current_turn));
    }
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
