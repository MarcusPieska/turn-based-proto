//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_SHIP_DEFENSE_BOOSTER_REGISTER_H
#define CITY_SHIP_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityShipDefenseBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped SHIP_DEFENSE booster register.
//
//================================================================================================================================

class CityShipDefenseBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityShipDefenseBoosterRegister () = delete;
    CityShipDefenseBoosterRegister (const CityShipDefenseBoosterRegister& other) = delete;
    CityShipDefenseBoosterRegister (CityShipDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_SHIP_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
