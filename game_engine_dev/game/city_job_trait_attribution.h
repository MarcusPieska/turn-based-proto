//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_JOB_TRAIT_ATTRIBUTION_H
#define CITY_JOB_TRAIT_ATTRIBUTION_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class CityJobStaticData;
class TraitAffinityMap;

//================================================================================================================================
//=> - CityJobTraitAttribution -
//================================================================================================================================
//
//  Static multi-tag attribution of city jobs to CivTrait bits from positive yield columns (scaled by
//  amount) plus ItemEffectsStruct boosters/produces, resolved through TraitAffinityMap. begin scans
//  CityJobStaticData once; mask bit i is CivTrait raw i; base() sums mapped token m_base values.
//
//================================================================================================================================

class CityJobTraitAttribution {
public:
    CityJobTraitAttribution () = delete;

    static bool begin (const CityJobStaticData& jobs, const TraitAffinityMap& aff);
    static void clear ();
    static bool ready ();
    static u16 job_n ();
    static u8 mask (u16 job_idx);
    static u16 base (u16 job_idx);
    static bool has (u16 job_idx, CivTrait trait);
    static bool add_tag (u16 job_idx, CivTrait trait);
    static u16 count_for (CivTrait trait);

private:
    static u8* m_masks;
    static u16* m_bases;
    static u16 m_n;
};

#endif // CITY_JOB_TRAIT_ATTRIBUTION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
