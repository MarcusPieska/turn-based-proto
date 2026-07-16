//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_POP_GROWTH_BOOSTER_REGISTER_H
#define LOCAL_POP_GROWTH_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalPopGrowthBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped POP_GROWTH booster register.
//
//================================================================================================================================

class LocalPopGrowthBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_local, ctx);
    }

private:
    LocalPopGrowthBoosterRegister () = delete;
    LocalPopGrowthBoosterRegister (const LocalPopGrowthBoosterRegister& other) = delete;
    LocalPopGrowthBoosterRegister (LocalPopGrowthBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POP_GROWTH;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_POP_GROWTH_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
