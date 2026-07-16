//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_CULTURE_BOOSTER_REGISTER_H
#define LOCAL_CULTURE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalCultureBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped CULTURE booster register.
//
//================================================================================================================================

class LocalCultureBoosterRegister : public BoosterEffectRegister {
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
    LocalCultureBoosterRegister () = delete;
    LocalCultureBoosterRegister (const LocalCultureBoosterRegister& other) = delete;
    LocalCultureBoosterRegister (LocalCultureBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CULTURE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_CULTURE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
