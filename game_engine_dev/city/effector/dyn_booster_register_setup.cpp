//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_booster_register_setup.h"

#include "building_static_key.h"
#include "effect_enabler.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static void tally_boost (const ItemEffectsStruct& fx, u16* counts, u16& total) {
    const u16 want = static_cast<u16>(ItemEffectType::BOOSTER);
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const ItemEffectBooster& b = fx.items[j].effect.booster;
        const u16 tp = static_cast<u16>(b.target_id);
        const u16 sc = static_cast<u16>(b.scope);
        if (tp >= DynBoosterRegister::TYPE_N || sc >= DynBoosterRegister::SCOPE_N) {
            continue;
        }
        if (sc == static_cast<u16>(ItemEffectsScope::NONE) || tp == static_cast<u16>(ItemEffectBoosterType::NONE)) {
            continue;
        }
        const u16 key = static_cast<u16>(tp * DynBoosterRegister::SCOPE_N + sc);
        ++counts[key];
        ++total;
    }
}

static void fill_boost (const ItemEffectsStruct& fx, EffectEnablerKind kind, u16 host_idx,
    BoosterRegisterEntry* entry, u16* curs) {
    const u16 want = static_cast<u16>(ItemEffectType::BOOSTER);
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const ItemEffectBooster& b = fx.items[j].effect.booster;
        const u16 tp = static_cast<u16>(b.target_id);
        const u16 sc = static_cast<u16>(b.scope);
        if (tp >= DynBoosterRegister::TYPE_N || sc >= DynBoosterRegister::SCOPE_N) {
            continue;
        }
        if (sc == static_cast<u16>(ItemEffectsScope::NONE) || tp == static_cast<u16>(ItemEffectBoosterType::NONE)) {
            continue;
        }
        const u16 key = static_cast<u16>(tp * DynBoosterRegister::SCOPE_N + sc);
        BoosterRegisterEntry& e = entry[curs[key]++];
        e.m_enabler.m_kind = kind;
        e.m_enabler.m_idx = host_idx;
        e.m_unit = (b.amount_mode == ItemEffectAmountMode::COUNT) ? b.amount : 0;
        e.m_perc = (b.amount_mode == ItemEffectAmountMode::PERCENTAGE) ? b.amount : 0;
    }
}

//================================================================================================================================
//=> - DynBoosterRegisterSetup -
//================================================================================================================================

bool DynBoosterRegisterSetup::build (const RuntimeStatics& st, DynBoosterRegister& out) {
    out.clear();
    u16* counts = new u16[DynBoosterRegister::KEY_N]();
    u16 total = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        tally_boost(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects, counts, total);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        tally_boost(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects, counts, total);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        tally_boost(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects, counts, total);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        tally_boost(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects, counts, total);
    }
    out.m_entry_n = total;
    out.m_off = new u16[DynBoosterRegister::KEY_N + 1u];
    out.m_off[0] = 0;
    for (u16 k = 0; k < DynBoosterRegister::KEY_N; ++k) {
        out.m_off[k + 1u] = static_cast<u16>(out.m_off[k] + counts[k]);
    }
    out.m_entry = (total > 0) ? new BoosterRegisterEntry[total] : nullptr;
    u16* curs = new u16[DynBoosterRegister::KEY_N];
    for (u16 k = 0; k < DynBoosterRegister::KEY_N; ++k) {
        curs[k] = out.m_off[k];
    }
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        fill_boost(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::BUILDING, i, out.m_entry, curs);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        fill_boost(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::TECH, i, out.m_entry, curs);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        fill_boost(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::SMALL_WONDER, i, out.m_entry, curs);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        fill_boost(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::WONDER, i, out.m_entry, curs);
    }
    delete[] curs;
    delete[] counts;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
