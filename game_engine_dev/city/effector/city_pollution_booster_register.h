//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_POLLUTION_BOOSTER_REGISTER_H
#define CITY_POLLUTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityPollutionBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped POLLUTION booster register.
//
//================================================================================================================================

class CityPollutionBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 3;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityPollutionBoosterRegister () = delete;
    CityPollutionBoosterRegister (const CityPollutionBoosterRegister& other) = delete;
    CityPollutionBoosterRegister (CityPollutionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::POLLUTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[3];
};

#endif // CITY_POLLUTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
