//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "effect_enabler.h"

#include "effect_ctx.h"
#include "bit_array.h"
#include "general_bit_bank.h"

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool chk_tech (const EffectCtx& ctx, u16 ix) {
    if (ctx.m_tech == nullptr || ix >= ctx.m_tech->get_count()) {
        return false;
    }
    return ctx.m_tech->get_bit(ix) == 1;
}

static bool chk_bld_city (const EffectCtx& ctx, u16 ix) {
    if (ctx.m_bld_bank == nullptr) {
        return false;
    }
    return ctx.m_bld_bank->is_flagged(ctx.m_city_idx, ix);
}

static bool chk_bld_empire (const EffectCtx& ctx, u16 ix) {
    if (ctx.m_emp_bld_fn != nullptr) {
        return ctx.m_emp_bld_fn(ctx.m_owner, ix, ctx.m_emp_bld_ud);
    }
    return chk_bld_city(ctx, ix);
}

static bool chk_small_wonder (const EffectCtx& ctx, u16 ix) {
    if (ctx.m_small_wonder_city == nullptr || ix >= ctx.m_small_wonder_n) {
        return false;
    }
    return ctx.m_small_wonder_city[ix] != U16_KEY_NULL;
}

static bool chk_wonder (const EffectCtx& ctx, u16 ix) {
    if (ctx.m_wonder_city == nullptr || ix >= ctx.m_wonder_n) {
        return false;
    }
    return ctx.m_wonder_city[ix] != U16_KEY_NULL;
}

static bool active_with_bld_fn (
    const EffectEnabler& en,
    const EffectCtx& ctx,
    bool (*bld_fn) (const EffectCtx&, u16)
) {
    switch (en.m_kind) {
    case EffectEnablerKind::BUILDING:
        return bld_fn(ctx, en.m_idx);
    case EffectEnablerKind::TECH:
        return chk_tech(ctx, en.m_idx);
    case EffectEnablerKind::SMALL_WONDER:
        return chk_small_wonder(ctx, en.m_idx);
    case EffectEnablerKind::WONDER:
        return chk_wonder(ctx, en.m_idx);
    default:
        return false;
    }
}

//================================================================================================================================
//=> - EffectEnabler -
//================================================================================================================================

bool effect_enabler_active_local (const EffectEnabler& en, const EffectCtx& ctx) {
    return active_with_bld_fn(en, ctx, chk_bld_city);
}

bool effect_enabler_active_city (const EffectEnabler& en, const EffectCtx& ctx) {
    return active_with_bld_fn(en, ctx, chk_bld_city);
}

bool effect_enabler_active_civ (const EffectEnabler& en, const EffectCtx& ctx) {
    return active_with_bld_fn(en, ctx, chk_bld_empire);
}

bool effect_enabler_active_global (const EffectEnabler& en, const EffectCtx& ctx) {
    return active_with_bld_fn(en, ctx, chk_bld_empire);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
