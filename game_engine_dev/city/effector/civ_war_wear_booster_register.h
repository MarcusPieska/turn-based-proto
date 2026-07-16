//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_WAR_WEAR_BOOSTER_REGISTER_H
#define CIV_WAR_WEAR_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivWarWearBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped WAR_WEAR booster register.
//
//================================================================================================================================

class CivWarWearBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 2;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivWarWearBoosterRegister () = delete;
    CivWarWearBoosterRegister (const CivWarWearBoosterRegister& other) = delete;
    CivWarWearBoosterRegister (CivWarWearBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::WAR_WEAR;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[2];
};

#endif // CIV_WAR_WEAR_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
