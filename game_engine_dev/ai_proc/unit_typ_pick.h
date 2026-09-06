//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_TYP_PICK_H
#define UNIT_TYP_PICK_H

#include "game_primitives.h"

class BitArrayCL;
class GameState;

//================================================================================================================================
//=> - UnitTypPick -
//================================================================================================================================
//
//  Picks a trainable unit catalog index for a unit-type. Current strategy: assessor bitset already
//  filled by the caller, then a right-to-left linear scan (newest catalog rows first). A second
//  ordered-scan strategy can live alongside this later without changing call sites.
//
//================================================================================================================================

class UnitTypPick {
public:
    UnitTypPick () = delete;

    static u16 pick_linear_right (const GameState& state, const BitArrayCL* available, u16 type_idx);
};

#endif // UNIT_TYP_PICK_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
