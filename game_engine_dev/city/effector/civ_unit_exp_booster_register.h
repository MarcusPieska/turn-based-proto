//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_UNIT_EXP_BOOSTER_REGISTER_H
#define CIV_UNIT_EXP_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivUnitExpBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped UNIT_EXP booster register.
//
//================================================================================================================================

class CivUnitExpBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivUnitExpBoosterRegister () = delete;
    CivUnitExpBoosterRegister (const CivUnitExpBoosterRegister& other) = delete;
    CivUnitExpBoosterRegister (CivUnitExpBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UNIT_EXP;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_UNIT_EXP_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
