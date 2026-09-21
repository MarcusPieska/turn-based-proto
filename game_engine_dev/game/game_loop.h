//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_LOOP_H
#define GAME_LOOP_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - GameLoop -
//================================================================================================================================
//
//  Per-turn match stepper. begin binds state, opens the runtime trace, inits whiteboards and SettlerTurnHandler,
//  arms settling targets, ResearchTurnHandler::begin for first tech targets, and claims starting city borders;
//  end tears those down. The external driver calls step. Each step: cities (CityTurnHandler), then units; after
//  unit upkeep settle, LedgerBalanceTurnHandler sets science %, pay from m_commerce_from_turn, then research bank.
//
//================================================================================================================================

class GameLoop {
public: 
    GameLoop ();
    ~GameLoop ();

    bool begin (GameState* state, cstr trace_path);
    bool step ();
    void end ();

private:
    GameState* m_state; // Bound match; null until begin

    GameLoop (const GameLoop& other) = delete;
    GameLoop& operator= (const GameLoop& other) = delete;
    GameLoop (GameLoop&& other) = delete;
    GameLoop& operator= (GameLoop&& other) = delete;
};

#endif // GAME_LOOP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
