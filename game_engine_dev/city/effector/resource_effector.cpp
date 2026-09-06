//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "resource_effector.h"

#include "bit_array.h"
#include "booster_apply.h"
#include "building_static_key.h"
#include "general_bit_bank.h"
#include "resource_turn_handler.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "tile_imp_helper.h"
#include "wonder_static_key.h"
#include "worker_job_imp_static_key.h"

//================================================================================================================================
//=> - Statics -
//================================================================================================================================

ResourceEffectEntry* ResourceEffector::m_any = nullptr;
u16 ResourceEffector::m_any_n = 0;
ResourceEffectEntry* ResourceEffector::m_entry = nullptr;
u16* ResourceEffector::m_off = nullptr;
u16 ResourceEffector::m_entry_n = 0;
u16 ResourceEffector::m_res_n = 0;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static bool is_res_boost (const ItemEffectStruct& slot) {
    if (slot.type != static_cast<u16>(ItemEffectType::BOOSTER)) {
        return false;
    }
    const ItemEffectBooster& b = slot.effect.booster;
    if (b.target_id != ItemEffectBoosterType::RESOURCE) {
        return false;
    }
    if (b.scope == ItemEffectsScope::NONE) {
        return false;
    }
    return true;
}

static void fill_entry (ResourceEffectEntry& e, ResourceEffectSrc src, u16 host_idx, const ItemEffectBooster& b) {
    e.m_src = src;
    e.m_idx = host_idx;
    e.m_unit = (b.amount_mode == ItemEffectAmountMode::COUNT) ? b.amount : 0;
    e.m_perc = (b.amount_mode == ItemEffectAmountMode::PERCENTAGE) ? b.amount : 0;
    e.m_scope = b.scope;
}

static u16 tally_res_boost (const ItemEffectsStruct& fx) {
    u16 n = 0;
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (is_res_boost(fx.items[j])) {
            ++n;
        }
    }
    return n;
}

static void append_res_boost (const ItemEffectsStruct& fx, ResourceEffectSrc src, u16 host_idx,
    ResourceEffectEntry* dst, u16& wri) {
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (!is_res_boost(fx.items[j])) {
            continue;
        }
        fill_entry(dst[wri++], src, host_idx, fx.items[j].effect.booster);
    }
}

//================================================================================================================================
//=> - ResourceEffector -
//================================================================================================================================

bool ResourceEffector::src_on (const ResourceEffectEntry& e, const ResourceExtractCtx& ctx) {
    switch (e.m_src) {
    case ResourceEffectSrc::TECH:
        if (ctx.m_tech == nullptr || e.m_idx >= ctx.m_tech->get_count()) {
            return false;
        }
        return ctx.m_tech->get_bit(e.m_idx) == 1;
    case ResourceEffectSrc::BUILDING:
        if (ctx.m_bld_bank == nullptr || ctx.m_city_idx == U16_KEY_NULL) {
            return false;
        }
        return ctx.m_bld_bank->is_flagged(ctx.m_city_idx, e.m_idx);
    case ResourceEffectSrc::SMALL_WONDER:
        if (ctx.m_small_wonder_city == nullptr || e.m_idx >= ctx.m_small_wonder_n) {
            return false;
        }
        if (ctx.m_city_idx == U16_KEY_NULL) {
            return ctx.m_small_wonder_city[e.m_idx] != U16_KEY_NULL;
        }
        return ctx.m_small_wonder_city[e.m_idx] == ctx.m_city_idx;
    case ResourceEffectSrc::WONDER:
        if (ctx.m_wonder_city == nullptr || e.m_idx >= ctx.m_wonder_n) {
            return false;
        }
        if (ctx.m_city_idx == U16_KEY_NULL) {
            return ctx.m_wonder_city[e.m_idx] != U16_KEY_NULL;
        }
        return ctx.m_wonder_city[e.m_idx] == ctx.m_city_idx;
    case ResourceEffectSrc::WORKER_JOB_IMP:
        if (ctx.m_tile == nullptr || ctx.m_statics == nullptr) {
            return false;
        }
        return TileImpHelper::has_imp(ctx.m_tile, *ctx.m_statics, e.m_idx);
    default:
        return false;
    }
}

void ResourceEffector::accum (const ResourceEffectEntry* rows, u16 n, const ResourceExtractCtx& ctx,
    i16& unit, i16& perc) {
    if (rows == nullptr || n == 0) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        const ResourceEffectEntry& e = rows[i];
        if (!src_on(e, ctx)) {
            continue;
        }
        unit = static_cast<i16>(static_cast<i32>(unit) + static_cast<i32>(e.m_unit));
        perc = static_cast<i16>(static_cast<i32>(perc) + static_cast<i32>(e.m_perc));
    }
}

bool ResourceEffector::setup (const RuntimeStatics& st) {
    clear();
    m_res_n = st.resource().get_item_count();
    u16 total = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        total = static_cast<u16>(total + tally_res_boost(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects));
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        total = static_cast<u16>(total + tally_res_boost(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects));
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        total = static_cast<u16>(total + tally_res_boost(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects));
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        total = static_cast<u16>(total + tally_res_boost(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects));
    }
    for (u16 i = 0; i < st.worker_job_imp().get_item_count(); ++i) {
        total = static_cast<u16>(total + tally_res_boost(st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(i)).effects));
    }
    m_any_n = total;
    if (total > 0) {
        m_any = new ResourceEffectEntry[total];
        if (m_any == nullptr) {
            clear();
            return false;
        }
    }
    u16 wri = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        append_res_boost(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects,
            ResourceEffectSrc::BUILDING, i, m_any, wri);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        append_res_boost(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects,
            ResourceEffectSrc::TECH, i, m_any, wri);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        append_res_boost(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects,
            ResourceEffectSrc::SMALL_WONDER, i, m_any, wri);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        append_res_boost(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects,
            ResourceEffectSrc::WONDER, i, m_any, wri);
    }
    for (u16 i = 0; i < st.worker_job_imp().get_item_count(); ++i) {
        append_res_boost(st.worker_job_imp().get_item(WorkerJobImpStaticDataKey::from_raw(i)).effects,
            ResourceEffectSrc::WORKER_JOB_IMP, i, m_any, wri);
    }
    m_off = new u16[m_res_n + 1u];
    if (m_off == nullptr) {
        clear();
        return false;
    }
    for (u16 i = 0; i <= m_res_n; ++i) {
        m_off[i] = 0;
    }
    m_entry_n = 0;
    m_entry = nullptr;
    return true;
}

void ResourceEffector::clear () {
    delete[] m_any;
    delete[] m_entry;
    delete[] m_off;
    m_any = nullptr;
    m_entry = nullptr;
    m_off = nullptr;
    m_any_n = 0;
    m_entry_n = 0;
    m_res_n = 0;
}

u16 ResourceEffector::yield (u16 res_idx, u16 base, const ResourceExtractCtx& ctx) {
    BoosterRegisterResult b = {};
    accum(m_any, m_any_n, ctx, b.m_unit, b.m_perc);
    if (m_off != nullptr && res_idx < m_res_n) {
        const u16 begin = m_off[res_idx];
        const u16 end = m_off[res_idx + 1u];
        accum(m_entry + begin, static_cast<u16>(end - begin), ctx, b.m_unit, b.m_perc);
    }
    return apply_booster_u16(base, b);
}

u16 ResourceEffector::any_n () {
    return m_any_n;
}

u16 ResourceEffector::entry_n () {
    return m_entry_n;
}

u16 ResourceEffector::res_n () {
    return m_res_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
