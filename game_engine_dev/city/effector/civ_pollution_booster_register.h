//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_POLLUTION_BOOSTER_REGISTER_H
#define CIV_POLLUTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivPollutionBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped POLLUTION booster register.
//
//================================================================================================================================

class CivPollutionBoosterRegister : public BoosterEffectRegister {
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
    CivPollutionBoosterRegister () = delete;
    CivPollutionBoosterRegister (const CivPollutionBoosterRegister& other) = delete;
    CivPollutionBoosterRegister (CivPollutionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POLLUTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_POLLUTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
