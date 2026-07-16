//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_POLLUTION_BOOSTER_REGISTER_H
#define LOCAL_POLLUTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalPollutionBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped POLLUTION booster register.
//
//================================================================================================================================

class LocalPollutionBoosterRegister : public BoosterEffectRegister {
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
    LocalPollutionBoosterRegister () = delete;
    LocalPollutionBoosterRegister (const LocalPollutionBoosterRegister& other) = delete;
    LocalPollutionBoosterRegister (LocalPollutionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POLLUTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_POLLUTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
