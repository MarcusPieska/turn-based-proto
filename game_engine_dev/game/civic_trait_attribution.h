//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIVIC_TRAIT_ATTRIBUTION_H
#define CIVIC_TRAIT_ATTRIBUTION_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class CivicStaticData;
class TraitAffinityMap;

//================================================================================================================================
//=> - CivicTraitAttribution -
//================================================================================================================================
//
//  Static multi-tag attribution of civics to CivTrait bits from ItemEffectsStruct boosters/produces,
//  resolved through TraitAffinityMap (token tags + per-token m_base). begin scans CivicStaticData
//  once; mask bit i corresponds to CivTrait with raw value i. base() is the summed m_base for the
//  civic's mapped effect tokens. add_tag can OR more tags after begin.
//
//================================================================================================================================

class CivicTraitAttribution {
public:
    CivicTraitAttribution () = delete;

    static bool begin (const CivicStaticData& civics, const TraitAffinityMap& aff);
    static void clear ();
    static bool ready ();
    static u16 civic_n ();
    static u8 mask (u16 civic_idx);
    static u16 base (u16 civic_idx);
    static bool has (u16 civic_idx, CivTrait trait);
    static bool add_tag (u16 civic_idx, CivTrait trait);
    static u16 count_for (CivTrait trait);

private:
    static u8* m_masks;
    static u16* m_bases;
    static u16 m_n;
};

#endif // CIVIC_TRAIT_ATTRIBUTION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
