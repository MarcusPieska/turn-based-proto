//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TARGET_SORT_BY_CORE_H
#define TARGET_SORT_BY_CORE_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - TargetSortByCore -
//================================================================================================================================
//
//  Sorts a city-index target list by Manhattan distance to the attacker land core (mean of that
//  seat's city coordinates). Builds a parallel key array on the stack and quicksorts both in lockstep.
//  Cap is TARGET_SORT_BY_CORE_CAP entries.
//
//================================================================================================================================

#define TARGET_SORT_BY_CORE_CAP 256u

class TargetSortByCore {
public:
    static void sort (const GameState& st, u8 attacker, u16* tgts, u16 n);

private:
    TargetSortByCore () = delete;

    static void qsort (u16* tgts, u32* keys, i32 lo, i32 hi);
    static i32 part (u16* tgts, u32* keys, i32 lo, i32 hi);
};

#endif // TARGET_SORT_BY_CORE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
