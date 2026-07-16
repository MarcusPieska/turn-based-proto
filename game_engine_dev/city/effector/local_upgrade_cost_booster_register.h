//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_UPGRADE_COST_BOOSTER_REGISTER_H
#define LOCAL_UPGRADE_COST_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalUpgradeCostBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped UPGRADE_COST booster register.
//
//================================================================================================================================

class LocalUpgradeCostBoosterRegister : public BoosterEffectRegister {
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
    LocalUpgradeCostBoosterRegister () = delete;
    LocalUpgradeCostBoosterRegister (const LocalUpgradeCostBoosterRegister& other) = delete;
    LocalUpgradeCostBoosterRegister (LocalUpgradeCostBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UPGRADE_COST;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_UPGRADE_COST_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
