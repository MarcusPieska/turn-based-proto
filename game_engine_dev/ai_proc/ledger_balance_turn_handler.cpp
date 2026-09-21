//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "ledger_balance_turn_handler.h"

#include "assert_log.h"
#include "game_state.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u16 ceil_pct_10 (u32 cost, u32 from) {
    if (from == 0u) {
        return 100u;
    }
    if (cost == 0u) {
        return 0u;
    }
    if (cost >= from) {
        return 100u;
    }
    const u32 pct = (cost * 100u + from - 1u) / from;
    const u32 step = ((pct + 9u) / 10u) * 10u;
    return step > 100u ? 100u : static_cast<u16>(step);
}

//================================================================================================================================
//=> - LedgerBalanceTurnHandler -
//================================================================================================================================

void LedgerBalanceTurnHandler::handle (GameState& state, u16 player) {
    GAME_EXPECT(state.m_player_states != nullptr, "LedgerBalanceTurnHandler null player states");
    GAME_EXPECT(player < state.m_player_n, "LedgerBalanceTurnHandler player out of bounds");
    PlayerState& ps = state.m_player_states[player];

    const u32 from = ps.m_commerce_from_turn;
    const u32 cost = static_cast<u32>(ps.m_land_unit_upkeep_needed) + static_cast<u32>(ps.m_naval_unit_upkeep_needed);
    // TODO: signed deficit + next-turn unit disband when cost > from even at 0% science
    if (from == 0u || cost >= from) {
        ps.m_research_spending_perc = 0u;
        return;
    }

    const u16 need_pct = ceil_pct_10(cost, from);
    u16 science = static_cast<u16>(100u - static_cast<u32>(need_pct));
    if (science >= 10u) {
        science = static_cast<u16>(science - 10u);
    } else {
        science = 0u;
    }
    // TODO: ease AI science further when ahead of the human in tech discoveries
    ps.m_research_spending_perc = science;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
