//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "effect_ctx.h"

#include "bit_array.h"
#include "general_bit_bank.h"
#include "item_effects.h"
#include "item_reqs.h"

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

static ItemEffectsScope fx_scope (const ItemEffectStruct& fx) {
    if (fx.type == static_cast<u16>(ItemEffectType::BOOSTER)) {
        return static_cast<ItemEffectsScope>(fx.effect.booster.scope);
    }
    return ItemEffectsScope::NONE;
}

static bool chk_bld_src (const EffectCtx& ctx, u16 ix, ItemEffectsScope sc) {
    if (sc == ItemEffectsScope::CITY || sc == ItemEffectsScope::LOCAL) {
        return chk_bld_city(ctx, ix);
    }
    return chk_bld_empire(ctx, ix);
}

//================================================================================================================================
//=> - EffectCtx queries -
//================================================================================================================================

bool effect_src_active (const EffectMapStruct& row, const EffectCtx& ctx) {
    const u16 ix = row.prereq_idx;
    if (ix == U16_KEY_NULL) {
        return false;
    }
    const ItemEffectsScope sc = fx_scope(row.effect);
    if (row.prereq_type == ITEM_REQ_TYPE_BUILDING) {
        return chk_bld_src(ctx, ix, sc);
    }
    if (row.prereq_type == ITEM_REQ_TYPE_TECH) {
        return chk_tech(ctx, ix);
    }
    if (row.prereq_type == static_cast<u16>(ITEM_REQ_TYPE_BUILDING + 1)) {
        return chk_small_wonder(ctx, ix);
    }
    if (row.prereq_type == static_cast<u16>(ITEM_REQ_TYPE_BUILDING + 2)) {
        return chk_wonder(ctx, ix);
    }
    return false;
}

bool effect_applies_to_city (const EffectMapStruct& row, const EffectCtx& ctx) {
    (void)ctx;
    const ItemEffectsScope sc = fx_scope(row.effect);
    if (sc == ItemEffectsScope::NONE) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
