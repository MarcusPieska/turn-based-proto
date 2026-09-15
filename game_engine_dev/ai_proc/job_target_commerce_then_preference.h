//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_TARGET_COMMERCE_THEN_PREFERENCE_H
#define JOB_TARGET_COMMERCE_THEN_PREFERENCE_H

#include "game_primitives.h"

struct EffectCtx;
class DynJobSlotRegister;
class DynJobYieldRegister;

//================================================================================================================================
//=> - JobTarget_CommerceThenPreference -
//================================================================================================================================
//
//  Fills commerce jobs first via DynJobYieldRegister::fill(COMMERCE), then remaining citizens by
//  CityJobTraitOrderings for trait_idx. Logs commerce phase and final pack; asserts pack.m_n cover.
//
//================================================================================================================================

class JobTarget_CommerceThenPreference {
public:
    static u16 fill (
        u16 pop_limit,
        u16 trait_idx,
        DynJobYieldRegister& yld,
        const DynJobSlotRegister& slots,
        const EffectCtx& ctx);

private:
    JobTarget_CommerceThenPreference () = delete;
};

#endif // JOB_TARGET_COMMERCE_THEN_PREFERENCE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
