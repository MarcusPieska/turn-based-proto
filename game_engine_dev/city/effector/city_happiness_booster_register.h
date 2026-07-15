//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_HAPPINESS_BOOSTER_REGISTER_H
#define CITY_HAPPINESS_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityHappinessBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped HAPPINESS booster register.
//
//================================================================================================================================

class CityHappinessBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 7;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityHappinessBoosterRegister () = delete;
    CityHappinessBoosterRegister (const CityHappinessBoosterRegister& other) = delete;
    CityHappinessBoosterRegister (CityHappinessBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::HAPPINESS;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_HAPPINESS_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
