//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LAND_POTENTIAL_H
#define LAND_POTENTIAL_H

#include "game_primitives.h"
#include "starting_point_generator.h"

class GameArraySimple;

//================================================================================================================================
//=> - LandPotential -
//================================================================================================================================
//
//  Scores each starting location by summing natural tile yields (food+production+commerce) in a
//  51x51 box centered on the start. Uses TileYields with a null-tech context (no resource unlocks).
//
//================================================================================================================================

class LandPotential {
public:
    static const u16 k_box = 51u; // Odd side length of the score window

    static bool score (
        const GameArraySimple& map,
        const SpgCoordPair* starts,
        u16 n,
        i32* out_scores);

private:
    LandPotential () = delete;
};

#endif // LAND_POTENTIAL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
