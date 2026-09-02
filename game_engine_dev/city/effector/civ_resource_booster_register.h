//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_RESOURCE_BOOSTER_REGISTER_H
#define CIV_RESOURCE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivResourceBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped RESOURCE booster register.
//
//================================================================================================================================

class CivResourceBoosterRegister : public BoosterEffectRegister {
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
    CivResourceBoosterRegister () = delete;
    CivResourceBoosterRegister (const CivResourceBoosterRegister& other) = delete;
    CivResourceBoosterRegister (CivResourceBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::RESOURCE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_RESOURCE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
