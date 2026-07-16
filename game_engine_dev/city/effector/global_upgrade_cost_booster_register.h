//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_UPGRADE_COST_BOOSTER_REGISTER_H
#define GLOBAL_UPGRADE_COST_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalUpgradeCostBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped UPGRADE_COST booster register.
//
//================================================================================================================================

class GlobalUpgradeCostBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_global, ctx);
    }

private:
    GlobalUpgradeCostBoosterRegister () = delete;
    GlobalUpgradeCostBoosterRegister (const GlobalUpgradeCostBoosterRegister& other) = delete;
    GlobalUpgradeCostBoosterRegister (GlobalUpgradeCostBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UPGRADE_COST;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_UPGRADE_COST_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
