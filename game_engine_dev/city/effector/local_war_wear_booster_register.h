//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_WAR_WEAR_BOOSTER_REGISTER_H
#define LOCAL_WAR_WEAR_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalWarWearBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped WAR_WEAR booster register.
//
//================================================================================================================================

class LocalWarWearBoosterRegister : public BoosterEffectRegister {
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
    LocalWarWearBoosterRegister () = delete;
    LocalWarWearBoosterRegister (const LocalWarWearBoosterRegister& other) = delete;
    LocalWarWearBoosterRegister (LocalWarWearBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::WAR_WEAR;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_WAR_WEAR_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
