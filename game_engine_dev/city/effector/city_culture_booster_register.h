//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_CULTURE_BOOSTER_REGISTER_H
#define CITY_CULTURE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityCultureBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped CULTURE booster register.
//
//================================================================================================================================

class CityCultureBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 49;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityCultureBoosterRegister () = delete;
    CityCultureBoosterRegister (const CityCultureBoosterRegister& other) = delete;
    CityCultureBoosterRegister (CityCultureBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::CULTURE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_CULTURE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
