//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_CORRUPTION_BOOSTER_REGISTER_H
#define CITY_CORRUPTION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityCorruptionBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped CORRUPTION booster register.
//
//================================================================================================================================

class CityCorruptionBoosterRegister : public BoosterEffectRegister {
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
    CityCorruptionBoosterRegister () = delete;
    CityCorruptionBoosterRegister (const CityCorruptionBoosterRegister& other) = delete;
    CityCorruptionBoosterRegister (CityCorruptionBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CORRUPTION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[2];
};

#endif // CITY_CORRUPTION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
