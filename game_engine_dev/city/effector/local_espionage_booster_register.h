//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_ESPIONAGE_BOOSTER_REGISTER_H
#define LOCAL_ESPIONAGE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalEspionageBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped ESPIONAGE booster register.
//
//================================================================================================================================

class LocalEspionageBoosterRegister : public BoosterEffectRegister {
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
    LocalEspionageBoosterRegister () = delete;
    LocalEspionageBoosterRegister (const LocalEspionageBoosterRegister& other) = delete;
    LocalEspionageBoosterRegister (LocalEspionageBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::ESPIONAGE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_ESPIONAGE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
