//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_SEA_TRADE_BOOSTER_REGISTER_H
#define LOCAL_SEA_TRADE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalSeaTradeBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped SEA_TRADE booster register.
//
//================================================================================================================================

class LocalSeaTradeBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 0;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_local, ctx);
    }

private:
    LocalSeaTradeBoosterRegister () = delete;
    LocalSeaTradeBoosterRegister (const LocalSeaTradeBoosterRegister& other) = delete;
    LocalSeaTradeBoosterRegister (LocalSeaTradeBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SEA_TRADE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_SEA_TRADE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
