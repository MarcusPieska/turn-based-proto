//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_COMMERCE_BOOSTER_REGISTER_H
#define CITY_COMMERCE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityCommerceBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped COMMERCE booster register.
//
//================================================================================================================================

class CityCommerceBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 6;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityCommerceBoosterRegister () = delete;
    CityCommerceBoosterRegister (const CityCommerceBoosterRegister& other) = delete;
    CityCommerceBoosterRegister (CityCommerceBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::COMMERCE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[6];
};

#endif // CITY_COMMERCE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
