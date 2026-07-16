//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GLOBAL_AIR_DEFENSE_BOOSTER_REGISTER_H
#define GLOBAL_AIR_DEFENSE_BOOSTER_REGISTER_H

#include "booster_effect_register.h"
#include "effect_enabler.h"

struct EffectCtx;

//================================================================================================================================
//=> - GlobalAirDefenseBoosterRegister -
//================================================================================================================================
//
//  Static GLOBAL-scoped AIR_DEFENSE booster register.
//
//================================================================================================================================

class GlobalAirDefenseBoosterRegister : public BoosterEffectRegister {
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
    GlobalAirDefenseBoosterRegister () = delete;
    GlobalAirDefenseBoosterRegister (const GlobalAirDefenseBoosterRegister& other) = delete;
    GlobalAirDefenseBoosterRegister (GlobalAirDefenseBoosterRegister&& other) = delete;

    static constexpr ItemEffectBoosterType booster_type () {
        return ItemEffectBoosterType::AIR_DEFENSE;
    }

    static constexpr ItemEffectsScope scope () {
        return ItemEffectsScope::GLOBAL;
    }

    static const BoosterRegisterEntry s_entry[1];
};

#endif // GLOBAL_AIR_DEFENSE_BOOSTER_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
