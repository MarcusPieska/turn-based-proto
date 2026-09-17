//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_TARGET_PREFERENCE_ONLY_H
#define JOB_TARGET_PREFERENCE_ONLY_H

#include "dyn_job_yield_register.h"
#include "game_primitives.h"

struct EffectCtx;
class DynJobSlotRegister;

//================================================================================================================================
//=> - JobTarget_PreferenceOnly -
//================================================================================================================================
//
//  Assigns citizens by CityJobTraitOrderings for trait_idx only. Uses DynJobYieldRegister remain/taken/pack
//  scratch (reset_remain then walk). fill_from_remain continues an existing pass without resetting.
//  fill returns the cumulative DynJobYieldPack (m_n = citizens assigned).
//
//================================================================================================================================

class JobTarget_PreferenceOnly {
public:
    static DynJobYieldPack fill (
        u16 pop_limit,
        u16 trait_idx,
        DynJobYieldRegister& yld,
        const DynJobSlotRegister& slots,
        const EffectCtx& ctx);

    static u16 fill_from_remain (
        u16 pop_limit,
        u16 trait_idx,
        DynJobYieldRegister& yld,
        const DynJobSlotRegister& slots,
        const EffectCtx& ctx);

private:
    JobTarget_PreferenceOnly () = delete;
};

#endif // JOB_TARGET_PREFERENCE_ONLY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
