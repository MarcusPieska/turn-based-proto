//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_TRAIT_ORDERINGS_H
#define BUILDING_TRAIT_ORDERINGS_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BitArrayCL;
class BuildingStaticData;

//================================================================================================================================
//=> - BuildingTraitOrderings -
//================================================================================================================================
//
//  Per-CivTrait preferred building index sequences. begin builds one full permutation of the building
//  catalog per trait using BuildingTraitAttribution tags and CivTraitAffinity preference rank.
//  pick returns the first available building in that trait's order (U16_KEY_NULL if none).
//
//================================================================================================================================

class BuildingTraitOrderings {
public:
    BuildingTraitOrderings () = delete;

    static bool begin (const BuildingStaticData& blds);
    static void clear ();
    static bool ready ();
    static u16 building_n ();
    static u16 at (u16 trait_idx, u16 slot);
    static u16 pick (const BitArrayCL& available, u16 trait_idx);

private:
    static u16* m_orders; // k_n rows of building_n indices
    static u16 m_n; // Building catalog size
};

#endif // BUILDING_TRAIT_ORDERINGS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
