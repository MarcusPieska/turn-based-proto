//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_SHIP_TRAINING_BOOSTER_REGISTER_H
#define CITY_SHIP_TRAINING_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityShipTrainingBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped SHIP_TRAINING booster register.
//
//================================================================================================================================

class CityShipTrainingBoosterRegister : public BoosterEffectRegister {
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
    CityShipTrainingBoosterRegister () = delete;
    CityShipTrainingBoosterRegister (const CityShipTrainingBoosterRegister& other) = delete;
    CityShipTrainingBoosterRegister (CityShipTrainingBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_TRAINING;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CITY_SHIP_TRAINING_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
