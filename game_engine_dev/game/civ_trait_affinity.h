//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_AFFINITY_H
#define CIV_TRAIT_AFFINITY_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

//================================================================================================================================
//=> - CivTraitAffinity -
//================================================================================================================================
//
//  Circular-buffer civ-trait affinity. One ring order defines proximity; from any owner the two nearest
//  neighbors are friendly, the next two neutral, and the two farthest hostile. Preference rank unrolls
//  that proximity (self, then cw/ccw pairs by distance). Used for building trait ranking and diplomacy.
//
//================================================================================================================================

class CivTraitAffinity {
public:
    static const u16 k_n = 7; // CivTrait catalog size

    CivTraitAffinity () = delete;

    static CivTrait pref (CivTrait owner, u16 rank);
    static u16 rank_of (CivTrait owner, CivTrait target);
    static i8 affinity (CivTrait owner, CivTrait target);
    static i32 received_score (CivTrait target);
    static void received_scores (i32* out_scores);
    static bool row_is_permutation (CivTrait owner);
    static CivTrait ring_at (u16 idx);
};

#endif // CIV_TRAIT_AFFINITY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
