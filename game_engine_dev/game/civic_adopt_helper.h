//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIVIC_ADOPT_HELPER_H
#define CIVIC_ADOPT_HELPER_H

#include "game_primitives.h"

class GameState;

//================================================================================================================================
//=> - CivicAdoptHelper -
//================================================================================================================================
//
//  Fills open civic slots for a seat up to STATIC_CIVIC_LIMIT + CIVIC_LIMIT boosters. Eligible civics
//  must meet tech prereqs, not already be adopted, and not share a CivicType with an adopted civic.
//  Picks via CivicTraitOrderings using the seat civ's primary trait.
//
//================================================================================================================================

class CivicAdoptHelper {
public:
    CivicAdoptHelper () = delete;

    static void fill (GameState& st, u16 seat);
};

#endif // CIVIC_ADOPT_HELPER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
