//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_TRAIT_ATTRIBUTION_H
#define BUILDING_TRAIT_ATTRIBUTION_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BuildingStaticData;
class TraitAffinityMap;

//================================================================================================================================
//=> - BuildingTraitAttribution -
//================================================================================================================================
//
//  Static multi-tag attribution of buildings to CivTrait bits from ItemEffectsStruct boosters/produces,
//  resolved through TraitAffinityMap (token tags + per-token m_base). begin scans BuildingStaticData
//  once; mask bit i corresponds to CivTrait with raw value i. base() is the summed m_base for the
//  building's mapped effect tokens. add_tag can OR more tags after begin.
//
//================================================================================================================================

class BuildingTraitAttribution {
public:
    BuildingTraitAttribution () = delete;

    static bool begin (const BuildingStaticData& blds, const TraitAffinityMap& aff);
    static void clear ();
    static bool ready ();
    static u16 building_n ();
    static u8 mask (u16 bld_idx);
    static u16 base (u16 bld_idx);
    static bool has (u16 bld_idx, CivTrait trait);
    static bool add_tag (u16 bld_idx, CivTrait trait);
    static u16 count_for (CivTrait trait);

private:
    static u8* m_masks;
    static u16* m_bases;
    static u16 m_n;
};

#endif // BUILDING_TRAIT_ATTRIBUTION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
