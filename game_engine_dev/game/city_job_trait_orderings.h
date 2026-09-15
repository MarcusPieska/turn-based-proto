//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_JOB_TRAIT_ORDERINGS_H
#define CITY_JOB_TRAIT_ORDERINGS_H

#include "civ_trait_enum.h"
#include "game_primitives.h"

class BitArrayCL;
class CityJobStaticData;
class TraitAffinityMap;

//================================================================================================================================
//=> - CityJobTraitOrderings -
//================================================================================================================================
//
//  Per-CivTrait preferred city-job index sequences. begin builds one full permutation of the job
//  catalog per trait from ring affinity plus TraitAffinityMap m_base (higher first). pick returns the
//  first available job in that trait's order (U16_KEY_NULL if none).
//
//================================================================================================================================

class CityJobTraitOrderings {
public:
    CityJobTraitOrderings () = delete;

    static bool begin (const CityJobStaticData& jobs, const TraitAffinityMap& aff);
    static void clear ();
    static bool ready ();
    static u16 job_n ();
    static u16 at (u16 trait_idx, u16 slot);
    static u16 pick (const BitArrayCL& available, u16 trait_idx, u16 start_slot = 0u);

private:
    static u16* m_orders;
    static u16 m_n;
};

#endif // CITY_JOB_TRAIT_ORDERINGS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
