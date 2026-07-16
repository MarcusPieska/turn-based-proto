//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_ESPIONAGE_BOOSTER_REGISTER_H
#define CITY_ESPIONAGE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityEspionageBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped ESPIONAGE booster register.
//
//================================================================================================================================

class CityEspionageBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 2;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityEspionageBoosterRegister () = delete;
    CityEspionageBoosterRegister (const CityEspionageBoosterRegister& other) = delete;
    CityEspionageBoosterRegister (CityEspionageBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::ESPIONAGE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[2];
};

#endif // CITY_ESPIONAGE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
