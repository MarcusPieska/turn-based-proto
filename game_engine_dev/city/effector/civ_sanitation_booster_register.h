//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_SANITATION_BOOSTER_REGISTER_H
#define CIV_SANITATION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivSanitationBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped SANITATION booster register.
//
//================================================================================================================================

class CivSanitationBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 8;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivSanitationBoosterRegister () = delete;
    CivSanitationBoosterRegister (const CivSanitationBoosterRegister& other) = delete;
    CivSanitationBoosterRegister (CivSanitationBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SANITATION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[8];
};

#endif // CIV_SANITATION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
