//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef [REGISTER_GUARD]
#define [REGISTER_GUARD]

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - [REGISTER_CLASS] -
//================================================================================================================================
//
//  Static [SCOPE_LABEL]-scoped [TYPE_LABEL] booster register.
//
//================================================================================================================================

class [REGISTER_CLASS] : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = [ENTRY_NUM];

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, [ACTIVE_FN], ctx);
    }

private:
    [REGISTER_CLASS] () = delete;
    [REGISTER_CLASS] (const [REGISTER_CLASS]& other) = delete;
    [REGISTER_CLASS] ([REGISTER_CLASS]&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::[TYPE_ENUM];
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::[SCOPE_ENUM];
    }

    static const BoosterRegisterEntry s_entry[[ENTRY_ARR_N]];
};

#endif // [REGISTER_GUARD]

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
