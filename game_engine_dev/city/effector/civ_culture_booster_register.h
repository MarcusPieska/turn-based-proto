//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_CULTURE_BOOSTER_REGISTER_H
#define CIV_CULTURE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivCultureBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped CULTURE booster register.
//
//================================================================================================================================

class CivCultureBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivCultureBoosterRegister () = delete;
    CivCultureBoosterRegister (const CivCultureBoosterRegister& other) = delete;
    CivCultureBoosterRegister (CivCultureBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CULTURE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_CULTURE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
