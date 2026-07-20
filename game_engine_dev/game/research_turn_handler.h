//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESEARCH_TURN_HANDLER_H
#define RESEARCH_TURN_HANDLER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - ResearchTurnHandler -
//================================================================================================================================
//
//  Per-seat research step. begin arms each seat's first LinearTech target. handle banks turn commerce into
//  research/treasury, then completes any affordable targets and picks the next.
//
//================================================================================================================================

class ResearchTurnHandler {
public:
    ResearchTurnHandler () = delete;

    static void begin (GameState& state);
    static void handle (GameState& state, u16 player);
};

#endif // RESEARCH_TURN_HANDLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
