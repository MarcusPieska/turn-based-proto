//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_UNIT_EXP_BOOSTER_REGISTER_H
#define GLOBAL_UNIT_EXP_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalUnitExpBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped UNIT_EXP booster register.
//
//================================================================================================================================

class GlobalUnitExpBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_global, ctx);
    }

private:
    GlobalUnitExpBoosterRegister () = delete;
    GlobalUnitExpBoosterRegister (const GlobalUnitExpBoosterRegister& other) = delete;
    GlobalUnitExpBoosterRegister (GlobalUnitExpBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UNIT_EXP;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_UNIT_EXP_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
