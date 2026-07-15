//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BOOSTER_EFFECT_REGISTER_H
#define BOOSTER_EFFECT_REGISTER_H

#include "effect_enabler.h"
#include "effect_index_selector.h"
#include "game_primitives.h"

struct EffectCtx;

//================================================================================================================================
//=> - BoosterRegisterEntry -
//================================================================================================================================

struct BoosterRegisterEntry {
    EffectEnabler m_enabler; // Source that must be active
    i16 m_unit; // COUNT-mode amount when active
    i16 m_perc; // PERCENTAGE-mode amount when active
};

//================================================================================================================================
//=> - BoosterRegisterResult -
//================================================================================================================================

struct BoosterRegisterResult {
    i16 m_unit; // Summed unit boost
    i16 m_perc; // Summed percentage boost
};

//================================================================================================================================
//=> - BoosterEffectRegister -
//================================================================================================================================
//
//  Static register base. Derived classes define s_entry[] and ENTRY_N.
//
//================================================================================================================================

class BoosterEffectRegister : public EffectIndexSelector {
protected:
    BoosterEffectRegister () = delete;

    static BoosterRegisterResult accum_entries (
        const BoosterRegisterEntry* entries,
        u16 entry_n,
        bool (*active_fn) (const EffectEnabler&, const EffectCtx&),
        const EffectCtx& ctx
    );

private:
    BoosterEffectRegister (const BoosterEffectRegister& other) = delete;
    BoosterEffectRegister (BoosterEffectRegister&& other) = delete;
};

#endif // BOOSTER_EFFECT_REGISTER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
