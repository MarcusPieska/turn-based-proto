//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TARGET_ORDERING_RIVER_SYSTEM_H
#define TARGET_ORDERING_RIVER_SYSTEM_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TargetOrderingRiverSystem -
//================================================================================================================================
//
//  From a seed river tile, uses GenWatershed to mark the river system, then appends enemy city
//  indices that sit on a system tile or are 8-adjacent to one. Returns how many were written.
//  Requires WhiteboardMng initialized to map size.
//
//================================================================================================================================

class TargetOrderingRiverSystem {
public:
    TargetOrderingRiverSystem ();

    u16 fill (GameState& st, u16 sx, u16 sy, u8 enemy, u16* out, u16 cap);

private:
};

#endif // TARGET_ORDERING_RIVER_SYSTEM_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
