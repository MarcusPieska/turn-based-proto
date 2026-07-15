//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_AIR_RANGE_BOOSTER_REGISTER_H
#define CITY_AIR_RANGE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityAirRangeBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped AIR_RANGE booster register.
//
//================================================================================================================================

class CityAirRangeBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityAirRangeBoosterRegister () = delete;
    CityAirRangeBoosterRegister (const CityAirRangeBoosterRegister& other) = delete;
    CityAirRangeBoosterRegister (CityAirRangeBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::AIR_RANGE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_AIR_RANGE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
