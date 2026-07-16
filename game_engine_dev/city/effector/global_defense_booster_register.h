//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_DEFENSE_BOOSTER_REGISTER_H
#define GLOBAL_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalDefenseBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped DEFENSE booster register.
//
//================================================================================================================================

class GlobalDefenseBoosterRegister : public BoosterEffectRegister {
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
    GlobalDefenseBoosterRegister () = delete;
    GlobalDefenseBoosterRegister (const GlobalDefenseBoosterRegister& other) = delete;
    GlobalDefenseBoosterRegister (GlobalDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
