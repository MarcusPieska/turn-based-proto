//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_PRODUCTION_BOOSTER_REGISTER_H
#define CITY_PRODUCTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityProductionBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped PRODUCTION booster register.
//
//================================================================================================================================

class CityProductionBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 9;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityProductionBoosterRegister () = delete;
    CityProductionBoosterRegister (const CityProductionBoosterRegister& other) = delete;
    CityProductionBoosterRegister (CityProductionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::PRODUCTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_PRODUCTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
