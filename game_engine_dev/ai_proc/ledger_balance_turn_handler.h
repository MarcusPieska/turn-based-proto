//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LEDGER_BALANCE_TURN_HANDLER_H
#define LEDGER_BALANCE_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - LedgerBalanceTurnHandler -
//================================================================================================================================
//
//  After free-support settle: set m_research_spending_perc in 10% steps so turn income covers paid
//  upkeep, then drop science another 10% as a treasury buffer. GameLoop pays upkeep from
//  m_commerce_from_turn, then ResearchTurnHandler banks the remainder. All seats use this for now.
//  TODO: easier difficulties — soft-cap AI tech vs the human; favor larger armies and more wars
//  over snowball tech (territory pressure is more fun than a tech leap).
//  TODO: if 0% science still cannot cover upkeep, allow a signed treasury deficit and disband units
//  on the next unit turn (prefer units not covered by local free support) until balance recovers.
//
//================================================================================================================================

class LedgerBalanceTurnHandler {
public:
    LedgerBalanceTurnHandler () = delete;

    static void handle (GameState& state, u16 player);
};

#endif // LEDGER_BALANCE_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
