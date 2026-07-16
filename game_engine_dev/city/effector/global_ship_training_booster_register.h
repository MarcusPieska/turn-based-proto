//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_SHIP_TRAINING_BOOSTER_REGISTER_H
#define GLOBAL_SHIP_TRAINING_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalShipTrainingBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped SHIP_TRAINING booster register.
//
//================================================================================================================================

class GlobalShipTrainingBoosterRegister : public BoosterEffectRegister {
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
    GlobalShipTrainingBoosterRegister () = delete;
    GlobalShipTrainingBoosterRegister (const GlobalShipTrainingBoosterRegister& other) = delete;
    GlobalShipTrainingBoosterRegister (GlobalShipTrainingBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_TRAINING;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_SHIP_TRAINING_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
