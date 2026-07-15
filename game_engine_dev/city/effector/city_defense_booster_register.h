//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_DEFENSE_BOOSTER_REGISTER_H
#define CITY_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityDefenseBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped DEFENSE booster register.
//
//================================================================================================================================

class CityDefenseBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 4;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityDefenseBoosterRegister () = delete;
    CityDefenseBoosterRegister (const CityDefenseBoosterRegister& other) = delete;
    CityDefenseBoosterRegister (CityDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
