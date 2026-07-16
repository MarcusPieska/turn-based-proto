//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_UPGRADE_COST_BOOSTER_REGISTER_H
#define CITY_UPGRADE_COST_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityUpgradeCostBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped UPGRADE_COST booster register.
//
//================================================================================================================================

class CityUpgradeCostBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityUpgradeCostBoosterRegister () = delete;
    CityUpgradeCostBoosterRegister (const CityUpgradeCostBoosterRegister& other) = delete;
    CityUpgradeCostBoosterRegister (CityUpgradeCostBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::UPGRADE_COST;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CITY_UPGRADE_COST_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
