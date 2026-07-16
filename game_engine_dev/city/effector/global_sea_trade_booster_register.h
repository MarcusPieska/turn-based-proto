//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_SEA_TRADE_BOOSTER_REGISTER_H
#define GLOBAL_SEA_TRADE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalSeaTradeBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped SEA_TRADE booster register.
//
//================================================================================================================================

class GlobalSeaTradeBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_global, ctx);
    }

private:
    GlobalSeaTradeBoosterRegister () = delete;
    GlobalSeaTradeBoosterRegister (const GlobalSeaTradeBoosterRegister& other) = delete;
    GlobalSeaTradeBoosterRegister (GlobalSeaTradeBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SEA_TRADE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_SEA_TRADE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
