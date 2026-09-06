//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GENERATE_ACCESS_MASK_H
#define GENERATE_ACCESS_MASK_H

#include "game_primitives.h"

class GameState;
class Whiteboard_1B;

//================================================================================================================================
//=> - GenerateAccessMask -
//================================================================================================================================
//
//  Fills a 1B 0/1 land-access mask from GameState seat ownership and CivRelations.
//  generate: Open (1) = unowned, self seat, or other seat whose civ rel is not CIV_REL_PEACE.
//            Blocked (0) = owned by a seat at CIV_REL_PEACE (or invalid seat/civ).
//  generate_own_free: Open (1) = unowned or self seat only; all foreign ownership blocked.
//
//================================================================================================================================

class GenerateAccessMask {
public:
    static const u8 k_block = 0u;
    static const u8 k_open = 1u;

    static bool generate (const GameState& s, u16 self_seat, Whiteboard_1B& out);
    static bool generate_own_free (const GameState& s, u16 self_seat, Whiteboard_1B& out);

private:
    GenerateAccessMask () = delete;
    GenerateAccessMask (const GenerateAccessMask& other) = delete;
    GenerateAccessMask (GenerateAccessMask&& other) = delete;
};

#endif // GENERATE_ACCESS_MASK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
