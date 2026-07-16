//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CITY_SEA_TRADE_BOOSTER_REGISTER_H
#define CITY_SEA_TRADE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CitySeaTradeBoosterRegister -
//================================================================================================================================
//
//  Static CITY-scoped SEA_TRADE booster register.
//
//================================================================================================================================

class CitySeaTradeBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 3;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_city, ctx);
    }

private:
    CitySeaTradeBoosterRegister () = delete;
    CitySeaTradeBoosterRegister (const CitySeaTradeBoosterRegister& other) = delete;
    CitySeaTradeBoosterRegister (CitySeaTradeBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SEA_TRADE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CITY;
    }

    static const BoosterRegisterEntry s_entry[3];
};

#endif // CITY_SEA_TRADE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
