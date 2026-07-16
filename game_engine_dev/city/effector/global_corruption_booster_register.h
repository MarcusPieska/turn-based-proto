//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_CORRUPTION_BOOSTER_REGISTER_H
#define GLOBAL_CORRUPTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalCorruptionBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped CORRUPTION booster register.
//
//================================================================================================================================

class GlobalCorruptionBoosterRegister : public BoosterEffectRegister {
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
    GlobalCorruptionBoosterRegister () = delete;
    GlobalCorruptionBoosterRegister (const GlobalCorruptionBoosterRegister& other) = delete;
    GlobalCorruptionBoosterRegister (GlobalCorruptionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CORRUPTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_CORRUPTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
