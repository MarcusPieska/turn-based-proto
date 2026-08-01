//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_TRAIT_ORDERINGS_H
#define TECH_TRAIT_ORDERINGS_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BitArrayCL;
class BuildingStaticData;
class TechStaticData;

//================================================================================================================================
//=> - TechTraitOrderings -
//================================================================================================================================
//
//  Per-CivTrait preferred tech index sequences for research picks. begin ranks the full tech catalog
//  from TechTraitAttribution::score_tree (higher score first). pick returns the first available tech
//  in that trait's order (U16_KEY_NULL if none).
//
//================================================================================================================================

class TechTraitOrderings {
public:
    TechTraitOrderings () = delete;

    static bool begin (const TechStaticData& techs, const BuildingStaticData& blds);
    static void clear ();
    static bool ready ();
    static u16 tech_n ();
    static u16 at (u16 trait_idx, u16 slot);
    static u16 pick (const BitArrayCL& available, u16 trait_idx);

private:
    static u16* m_orders; // k_n rows of tech_n indices
    static u16 m_n; // Tech catalog size
};

#endif // TECH_TRAIT_ORDERINGS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
