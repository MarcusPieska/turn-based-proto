//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_POP_GROWTH_BOOSTER_REGISTER_H
#define GLOBAL_POP_GROWTH_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalPopGrowthBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped POP_GROWTH booster register.
//
//================================================================================================================================

class GlobalPopGrowthBoosterRegister : public BoosterEffectRegister {
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
    GlobalPopGrowthBoosterRegister () = delete;
    GlobalPopGrowthBoosterRegister (const GlobalPopGrowthBoosterRegister& other) = delete;
    GlobalPopGrowthBoosterRegister (GlobalPopGrowthBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POP_GROWTH;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_POP_GROWTH_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
