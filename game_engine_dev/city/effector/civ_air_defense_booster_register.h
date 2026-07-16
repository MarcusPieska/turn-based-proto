//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_AIR_DEFENSE_BOOSTER_REGISTER_H
#define CIV_AIR_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - CivAirDefenseBoosterRegister -
//================================================================================================================================
//
//  Static CIV-scoped AIR_DEFENSE booster register.
//
//================================================================================================================================

class CivAirDefenseBoosterRegister : public BoosterEffectRegister {
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
    CivAirDefenseBoosterRegister () = delete;
    CivAirDefenseBoosterRegister (const CivAirDefenseBoosterRegister& other) = delete;
    CivAirDefenseBoosterRegister (CivAirDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::AIR_DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::CIV;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // CIV_AIR_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
