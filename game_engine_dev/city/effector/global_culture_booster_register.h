//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_CULTURE_BOOSTER_REGISTER_H
#define GLOBAL_CULTURE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalCultureBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped CULTURE booster register.
//
//================================================================================================================================

class GlobalCultureBoosterRegister : public BoosterEffectRegister {
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
    GlobalCultureBoosterRegister () = delete;
    GlobalCultureBoosterRegister (const GlobalCultureBoosterRegister& other) = delete;
    GlobalCultureBoosterRegister (GlobalCultureBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CULTURE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_CULTURE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
