//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_TRAIT_ATTRIBUTION_H
#define TECH_TRAIT_ATTRIBUTION_H

#include "building_static_data.h"
#include "civ_trait_enum.h"
#include "game_primitives.h"
#include "tech_static_data.h"

//================================================================================================================================
//=> - Score knobs -
//================================================================================================================================

#define TECH_TRAIT_SCORE_PRIMARY 15
#define TECH_TRAIT_SCORE_FRIENDLY 10
#define TECH_TRAIT_SCORE_NEUTRAL 5
#define TECH_TRAIT_SCORE_HOSTILE 0
#define TECH_TRAIT_SCORE_STEP (-5)

//================================================================================================================================
//=> - TechTraitAttribution -
//================================================================================================================================
//
//  Techs inherit CivTrait tags from buildings they unlock. For a primary trait, each building tag
//  scores by affinity band (PRIMARY/FRIENDLY/NEUTRAL/HOSTILE); sums are additive, then bleed to
//  upstream tech prereqs by TECH_TRAIT_SCORE_STEP per hop (also additive across paths).
//
//================================================================================================================================

class TechTraitAttribution {
public:
    TechTraitAttribution () = delete;

    static bool begin (const TechStaticData& techs, const BuildingStaticData& blds);
    static void clear ();
    static bool ready ();
    static u16 tech_n ();
    static u8 mask (u16 tech_idx);
    static bool has (u16 tech_idx, CivTrait trait);
    static u16 count_for (CivTrait trait);
    static i32 band_score (CivTrait owner, CivTrait tag);
    static i32 local_score (u16 tech_idx, CivTrait primary);
    static void propagate (u16 tech_idx, i32 score, i32* scores);
    static void score_tree (CivTrait primary, i32* scores);

private:
    static const TechStaticDataStruct* m_items; // Tech rows for prereq walks
    static const BuildingStaticDataStruct* m_blds; // Building rows for unlock scans
    static u8* m_masks; // Per-tech union of unlocked building trait masks
    static u16 m_n; // Tech count
    static u16 m_bld_n; // Building count
};

#endif // TECH_TRAIT_ATTRIBUTION_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
