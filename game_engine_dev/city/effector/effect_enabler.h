//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef EFFECT_ENABLER_H
#define EFFECT_ENABLER_H

#include "game_primitives.h"

struct EffectCtx;

//================================================================================================================================
//=> - EffectEnablerKind -
//================================================================================================================================

enum class EffectEnablerKind : u8 {
    BUILDING = 0,
    TECH = 1,
    SMALL_WONDER = 2,
    WONDER = 3
};

//================================================================================================================================
//=> - EffectEnabler -
//================================================================================================================================
//
//  Static source key for testing whether one catalog item is active in EffectCtx.
//
//================================================================================================================================

struct EffectEnabler {
    EffectEnablerKind m_kind; // Catalog family for m_idx
    u16 m_idx; // Row index inside that catalog
};

bool effect_enabler_active_local (const EffectEnabler& en, const EffectCtx& ctx);
bool effect_enabler_active_city (const EffectEnabler& en, const EffectCtx& ctx);
bool effect_enabler_active_civ (const EffectEnabler& en, const EffectCtx& ctx);
bool effect_enabler_active_global (const EffectEnabler& en, const EffectCtx& ctx);

#endif // EFFECT_ENABLER_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
