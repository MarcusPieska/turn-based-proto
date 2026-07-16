//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_SHIP_TRAINING_BOOSTER_REGISTER_H
#define LOCAL_SHIP_TRAINING_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalShipTrainingBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped SHIP_TRAINING booster register.
//
//================================================================================================================================

class LocalShipTrainingBoosterRegister : public BoosterEffectRegister {
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
    LocalShipTrainingBoosterRegister () = delete;
    LocalShipTrainingBoosterRegister (const LocalShipTrainingBoosterRegister& other) = delete;
    LocalShipTrainingBoosterRegister (LocalShipTrainingBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_TRAINING;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_SHIP_TRAINING_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
