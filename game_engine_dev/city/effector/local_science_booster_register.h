//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_SCIENCE_BOOSTER_REGISTER_H
#define LOCAL_SCIENCE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalScienceBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped SCIENCE booster register.
//
//================================================================================================================================

class LocalScienceBoosterRegister : public BoosterEffectRegister {
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
    LocalScienceBoosterRegister () = delete;
    LocalScienceBoosterRegister (const LocalScienceBoosterRegister& other) = delete;
    LocalScienceBoosterRegister (LocalScienceBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SCIENCE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_SCIENCE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
