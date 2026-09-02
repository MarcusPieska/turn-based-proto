//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_TRAIT_ORDERINGS_H
#define BUILDING_TRAIT_ORDERINGS_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BitArrayCL;
class BuildingStaticData;
class TraitAffinityMap;

//================================================================================================================================
//=> - BuildingTraitOrderings -
//================================================================================================================================
//
//  Per-CivTrait preferred building index sequences. begin builds one full permutation of the building
//  catalog per trait from ring affinity plus TraitAffinityMap m_base (higher first). pick returns the
//  first available building in that trait's order (U16_KEY_NULL if none).
//
//================================================================================================================================

class BuildingTraitOrderings {
public:
    BuildingTraitOrderings () = delete;

    static bool begin (const BuildingStaticData& blds, const TraitAffinityMap& aff);
    static void clear ();
    static bool ready ();
    static u16 building_n ();
    static u16 at (u16 trait_idx, u16 slot);
    static u16 pick (const BitArrayCL& available, u16 trait_idx, u16 start_slot = 0u);

private:
    static u16* m_orders;
    static u16 m_n;
};

#endif // BUILDING_TRAIT_ORDERINGS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
