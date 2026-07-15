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
//  Turn driver for a match. Advances GameState from setup through m_turn_limit turns.
//  Writes runtime trace events for post-hoc replay; hand off after run completes.
//
//================================================================================================================================

class GameLoop {
public: 
    GameLoop ();
    ~GameLoop ();

    bool run (GameState* state, cstr trace_path);

private:
    GameLoop (const GameLoop& other) = delete;
    GameLoop& operator= (const GameLoop& other) = delete;
    GameLoop (GameLoop&& other) = delete;
    GameLoop& operator= (GameLoop&& other) = delete;
};

#endif // GAME_LOOP_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
