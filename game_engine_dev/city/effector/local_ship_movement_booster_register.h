//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef LOCAL_SHIP_MOVEMENT_BOOSTER_REGISTER_H
#define LOCAL_SHIP_MOVEMENT_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - LocalShipMovementBoosterRegister -
//================================================================================================================================
//
//  Static LOCAL-scoped SHIP_MOVEMENT booster register.
//
//================================================================================================================================

class LocalShipMovementBoosterRegister : public BoosterEffectRegister {
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
    LocalShipMovementBoosterRegister () = delete;
    LocalShipMovementBoosterRegister (const LocalShipMovementBoosterRegister& other) = delete;
    LocalShipMovementBoosterRegister (LocalShipMovementBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::SHIP_MOVEMENT;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::LOCAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // LOCAL_SHIP_MOVEMENT_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
