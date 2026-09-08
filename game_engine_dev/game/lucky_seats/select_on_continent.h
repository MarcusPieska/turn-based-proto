//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef SELECT_ON_CONTINENT_H
#define SELECT_ON_CONTINENT_H

#include "game_primitives.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - SelectOnContinent -
//================================================================================================================================
//
//  Picks one free start on a continent: land-potential score mapped to 0..SCORE_SCALE plus average
//  Euclidean distance to already-selected points mapped to 0..DIST_SCALE; highest sum wins.
//
//================================================================================================================================

class SelectOnContinent {
public:
    static bool pick (
        const SpgCoordPair* cands,
        const i32* scores,
        u16 cand_n,
        const SpgCoordPair* selected,
        u16 sel_n,
        u16* out_idx);

private:
    SelectOnContinent () = delete;
};

#endif // SELECT_ON_CONTINENT_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
