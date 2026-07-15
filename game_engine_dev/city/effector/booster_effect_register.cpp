//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "booster_effect_register.h"

#include "effect_ctx.h"

//================================================================================================================================
//=> - BoosterEffectRegister -
//================================================================================================================================

BoosterRegisterResult BoosterEffectRegister::accum_entries (
    const BoosterRegisterEntry* entries,
    u16 entry_n,
    bool (*active_fn) (const EffectEnabler&, const EffectCtx&),
    const EffectCtx& ctx
) {
    BoosterRegisterResult out = {};
    if (entries == nullptr || entry_n == 0 || active_fn == nullptr) {
        return out;
    }
    for (u16 i = 0; i < entry_n; ++i) {
        const BoosterRegisterEntry& e = entries[i];
        if (!active_fn(e.m_enabler, ctx)) {
            continue;
        }
        out.m_unit = static_cast<i16>(static_cast<i32>(out.m_unit) + static_cast<i32>(e.m_unit));
        out.m_perc = static_cast<i16>(static_cast<i32>(out.m_perc) + static_cast<i32>(e.m_perc));
    }
    return out;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
