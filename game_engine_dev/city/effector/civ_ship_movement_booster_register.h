//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_SHIP_MOVEMENT_BOOSTER_REGISTER_H
#define CIV_SHIP_MOVEMENT_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivShipMovementBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped SHIP_MOVEMENT booster register.
//
//================================================================================================================================

class CivShipMovementBoosterRegister : public BoosterEffectRegister {
public:
    static constexpr u16 ENTRY_N = 2;

    static BoosterRegisterResult determine_effect (const EffectCtx& ctx) {
        return accum_entries(s_entry, ENTRY_N, effect_enabler_active_civ, ctx);
    }

private:
    CivShipMovementBoosterRegister () = delete;
    CivShipMovementBoosterRegister (const CivShipMovementBoosterRegister& other) = delete;
    CivShipMovementBoosterRegister (CivShipMovementBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_MOVEMENT;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[ENTRY_N];
};

#endif // CIV_SHIP_MOVEMENT_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
