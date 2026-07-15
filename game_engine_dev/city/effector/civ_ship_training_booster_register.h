//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_SHIP_TRAINING_BOOSTER_REGISTER_H
#define CIV_SHIP_TRAINING_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivShipTrainingBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped SHIP_TRAINING booster register.
//
//================================================================================================================================

class CivShipTrainingBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivShipTrainingBoosterRegister () = delete;
    CivShipTrainingBoosterRegister (const CivShipTrainingBoosterRegister& other) = delete;
    CivShipTrainingBoosterRegister (CivShipTrainingBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_TRAINING;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CIV_SHIP_TRAINING_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
