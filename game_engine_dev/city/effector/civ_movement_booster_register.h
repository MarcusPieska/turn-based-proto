//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_MOVEMENT_BOOSTER_REGISTER_H
#define CIV_MOVEMENT_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivMovementBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped MOVEMENT booster register.
//
//================================================================================================================================

class CivMovementBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 1;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        if (ENTRY_N == 0) {
            (void)ctx;
            return {};
        }
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivMovementBoosterRegister () = delete;
    CivMovementBoosterRegister (const CivMovementBoosterRegister& other) = delete;
    CivMovementBoosterRegister (CivMovementBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::MOVEMENT;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_MOVEMENT_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
