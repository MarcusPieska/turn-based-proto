//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_NUKE_DEFENSE_BOOSTER_REGISTER_H
#define CITY_NUKE_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CityNukeDefenseBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped NUKE_DEFENSE booster register.
//
//================================================================================================================================

class CityNukeDefenseBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CityNukeDefenseBoosterRegister () = delete;
    CityNukeDefenseBoosterRegister (const CityNukeDefenseBoosterRegister& other) = delete;
    CityNukeDefenseBoosterRegister (CityNukeDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::NUKE_DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CITY_NUKE_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
