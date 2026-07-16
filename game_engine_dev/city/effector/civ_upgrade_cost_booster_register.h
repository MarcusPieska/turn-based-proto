//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_UPGRADE_COST_BOOSTER_REGISTER_H
#define CIV_UPGRADE_COST_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivUpgradeCostBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped UPGRADE_COST booster register.
//
//================================================================================================================================

class CivUpgradeCostBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivUpgradeCostBoosterRegister () = delete;
    CivUpgradeCostBoosterRegister (const CivUpgradeCostBoosterRegister& other) = delete;
    CivUpgradeCostBoosterRegister (CivUpgradeCostBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UPGRADE_COST;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_UPGRADE_COST_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
