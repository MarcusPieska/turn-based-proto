//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_DEFENSE_BOOSTER_REGISTER_H
#define LOCAL_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalDefenseBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped DEFENSE booster register.
//
//================================================================================================================================

class LocalDefenseBoosterRegister : public BoosterEffectRegister {
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
    LocalDefenseBoosterRegister () = delete;
    LocalDefenseBoosterRegister (const LocalDefenseBoosterRegister& other) = delete;
    LocalDefenseBoosterRegister (LocalDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
