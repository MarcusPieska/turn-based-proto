//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_TRAIT_ATTRIBUTION_H
#define BUILDING_TRAIT_ATTRIBUTION_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BuildingStaticData;

//================================================================================================================================
//=> - BuildingTraitAttribution -
//================================================================================================================================
//
//  Static multi-tag attribution of buildings to CivTrait bits from ItemEffectsStruct boosters/produces.
//  begin scans BuildingStaticData once; mask bit i corresponds to CivTrait with raw value i.
//  Effect sources may contribute several traits; add_tag can OR more tags after begin.
//
//================================================================================================================================

class BuildingTraitAttribution {
public:
    BuildingTraitAttribution () = delete;

    static bool begin (const BuildingStaticData& blds);
    static void clear ();
    static bool ready ();
    static u16 building_n ();
    static u8 mask (u16 bld_idx);
    static bool has (u16 bld_idx, CivTrait trait);
    static bool add_tag (u16 bld_idx, CivTrait trait);
    static u16 count_for (CivTrait trait);

private:
    static u8* m_masks; // Per-building CivTrait bit masks
    static u16 m_n; // Building count cached from begin
};

#endif // BUILDING_TRAIT_ATTRIBUTION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
