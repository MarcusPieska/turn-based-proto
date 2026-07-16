//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_WAR_WEAR_BOOSTER_REGISTER_H
#define GLOBAL_WAR_WEAR_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalWarWearBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped WAR_WEAR booster register.
//
//================================================================================================================================

class GlobalWarWearBoosterRegister : public BoosterEffectRegister {
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
    GlobalWarWearBoosterRegister () = delete;
    GlobalWarWearBoosterRegister (const GlobalWarWearBoosterRegister& other) = delete;
    GlobalWarWearBoosterRegister (GlobalWarWearBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::WAR_WEAR;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_WAR_WEAR_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
