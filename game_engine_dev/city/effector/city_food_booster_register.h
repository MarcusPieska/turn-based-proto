//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_FOOD_BOOSTER_REGISTER_H
#define CITY_FOOD_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityFoodBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped FOOD booster register.
//
//================================================================================================================================

class CityFoodBoosterRegister : public BoosterEffectRegister {
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
    CityFoodBoosterRegister () = delete;
    CityFoodBoosterRegister (const CityFoodBoosterRegister& other) = delete;
    CityFoodBoosterRegister (CityFoodBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::FOOD;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CITY_FOOD_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
