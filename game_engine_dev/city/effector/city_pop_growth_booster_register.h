//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_POP_GROWTH_BOOSTER_REGISTER_H
#define CITY_POP_GROWTH_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityPopGrowthBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped POP_GROWTH booster register.
//
//================================================================================================================================

class CityPopGrowthBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 4;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityPopGrowthBoosterRegister () = delete;
    CityPopGrowthBoosterRegister (const CityPopGrowthBoosterRegister& other) = delete;
    CityPopGrowthBoosterRegister (CityPopGrowthBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POP_GROWTH;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_POP_GROWTH_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
