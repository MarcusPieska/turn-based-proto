//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_UNIT_EXP_BOOSTER_REGISTER_H
#define LOCAL_UNIT_EXP_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalUnitExpBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped UNIT_EXP booster register.
//
//================================================================================================================================

class LocalUnitExpBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_local, ctx);
    }

private:
    LocalUnitExpBoosterRegister () = delete;
    LocalUnitExpBoosterRegister (const LocalUnitExpBoosterRegister& other) = delete;
    LocalUnitExpBoosterRegister (LocalUnitExpBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UNIT_EXP;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // LOCAL_UNIT_EXP_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
