//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_SANITATION_BOOSTER_REGISTER_H
#define CITY_SANITATION_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CitySanitationBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped SANITATION booster register.
//
//================================================================================================================================

class CitySanitationBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 20;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CitySanitationBoosterRegister () = delete;
    CitySanitationBoosterRegister (const CitySanitationBoosterRegister& other) = delete;
    CitySanitationBoosterRegister (CitySanitationBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SANITATION;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[20];
};

#endif // CITY_SANITATION_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
