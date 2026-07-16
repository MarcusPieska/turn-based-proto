//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_SEA_TRADE_BOOSTER_REGISTER_H
#define CIV_SEA_TRADE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivSeaTradeBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped SEA_TRADE booster register.
//
//================================================================================================================================

class CivSeaTradeBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivSeaTradeBoosterRegister () = delete;
    CivSeaTradeBoosterRegister (const CivSeaTradeBoosterRegister& other) = delete;
    CivSeaTradeBoosterRegister (CivSeaTradeBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SEA_TRADE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_SEA_TRADE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
