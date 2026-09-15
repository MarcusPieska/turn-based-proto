//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef JOB_TARGET_PREFERENCE_ONLY_H
#define JOB_TARGET_PREFERENCE_ONLY_H

#include "game_primitives.h"

struct EffectCtx;
class DynJobSlotRegister;
class DynJobYieldRegister;

//================================================================================================================================
//=> - JobTarget_PreferenceOnly -
//================================================================================================================================
//
//  Assigns citizens by CityJobTraitOrderings for trait_idx only. Uses DynJobYieldRegister remain/pack
//  scratch (reset_remain then walk). fill_from_remain continues an existing pass without resetting.
//
//================================================================================================================================

class JobTarget_PreferenceOnly {
public:
    static u16 fill (
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
