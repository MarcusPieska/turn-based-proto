//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_AIR_DEFENSE_BOOSTER_REGISTER_H
#define CITY_AIR_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityAirDefenseBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped AIR_DEFENSE booster register.
//
//================================================================================================================================

class CityAirDefenseBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityAirDefenseBoosterRegister () = delete;
    CityAirDefenseBoosterRegister (const CityAirDefenseBoosterRegister& other) = delete;
    CityAirDefenseBoosterRegister (CityAirDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::AIR_DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_AIR_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
