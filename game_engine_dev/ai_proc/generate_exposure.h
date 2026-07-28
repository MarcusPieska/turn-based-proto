//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_EXPOSURE_H
#define GENERATE_EXPOSURE_H

#include "game_primitives.h"

class GameState;
class Whiteboard_1B;

//================================================================================================================================
//=> - GenerateExposure -
//================================================================================================================================
//
//  Owns-tile BFS distance from the self/enemy border. Seeds every self-owned passable tile that
//  shares an edge with an enemy-owned tile at 0, then floods across self tiles. Water and mountains
//  block. Unreached stay k_none (255).
//
//================================================================================================================================

class GenerateExposure {
public:
    static const u8 k_none = 0xFFu;

    static bool generate (const GameState& s, u16 seat, u16 enemy, Whiteboard_1B& out);

private:
    GenerateExposure () = delete;
};

#endif // GENERATE_EXPOSURE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
