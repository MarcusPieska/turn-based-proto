//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_PRODUCTION_BOOSTER_REGISTER_H
#define LOCAL_PRODUCTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalProductionBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped PRODUCTION booster register.
//
//================================================================================================================================

class LocalProductionBoosterRegister : public BoosterEffectRegister {
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
    LocalProductionBoosterRegister () = delete;
    LocalProductionBoosterRegister (const LocalProductionBoosterRegister& other) = delete;
    LocalProductionBoosterRegister (LocalProductionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::PRODUCTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_PRODUCTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
