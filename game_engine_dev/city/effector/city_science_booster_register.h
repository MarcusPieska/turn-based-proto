//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_SCIENCE_BOOSTER_REGISTER_H
#define CITY_SCIENCE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityScienceBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped SCIENCE booster register.
//
//================================================================================================================================

class CityScienceBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 8;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityScienceBoosterRegister () = delete;
    CityScienceBoosterRegister (const CityScienceBoosterRegister& other) = delete;
    CityScienceBoosterRegister (CityScienceBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SCIENCE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[8];
};

#endif // CITY_SCIENCE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
