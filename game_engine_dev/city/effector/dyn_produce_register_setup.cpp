//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_produce_register_setup.h"

#include "building_static_key.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u8 count_produce (const ItemEffectsStruct& fx) {
    const u16 want = static_cast<u16>(ItemEffectType::PRODUCE);
    u8 n = 0;
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type == want) {
            ++n;
        }
    }
    return n;
}

static void mark_res (const ItemEffectsStruct& fx, u8* used, u16 res_n) {
    const u16 want = static_cast<u16>(ItemEffectType::PRODUCE);
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const ItemEffectProduce& p = fx.items[j].effect.produce;
        if (p.kind != ItemProduceKind::RESOURCE || p.target_id >= res_n) {
            continue;
        }
        used[p.target_id] = 1;
    }
}

static void fill_slots (const ItemEffectsStruct& fx, DynProduceGroup& g, const u16* res_map, u16 res_n) {
    const u16 want = static_cast<u16>(ItemEffectType::PRODUCE);
    g.m_slot_n = 0;
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const ItemEffectProduce& p = fx.items[j].effect.produce;
        DynProduceSlot& sl = g.m_slots[g.m_slot_n++];
        sl.m_kind = p.kind;
        sl.m_amount = p.amount;
        if (p.kind == ItemProduceKind::RESOURCE) {
            sl.m_target = (p.target_id < res_n) ? res_map[p.target_id] : U16_KEY_NULL;
        } else {
            sl.m_target = p.target_id;
        }
    }
}

//================================================================================================================================
//=> - DynProduceRegisterSetup -
//================================================================================================================================

bool DynProduceRegisterSetup::build (const RuntimeStatics& st, DynProduceRegister& out) {
    out.clear();
    out.m_res_n = st.resource().get_item_count();
    u8* used = (out.m_res_n > 0) ? new u8[out.m_res_n]() : nullptr;
    u16 total = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        ++total;
        mark_res(fx, used, out.m_res_n);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.tech().get_item(TechStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        ++total;
        mark_res(fx, used, out.m_res_n);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        ++total;
        mark_res(fx, used, out.m_res_n);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        ++total;
        mark_res(fx, used, out.m_res_n);
    }

    u16 dense_n = 0;
    for (u16 i = 0; i < out.m_res_n; ++i) {
        if (used != nullptr && used[i] != 0) {
            ++dense_n;
        }
    }
    out.m_inv_n = dense_n;
    out.m_res_map = (out.m_res_n > 0) ? new u16[out.m_res_n] : nullptr;
    for (u16 i = 0; i < out.m_res_n; ++i) {
        out.m_res_map[i] = U16_KEY_NULL;
    }
    out.m_res_ids = (dense_n > 0) ? new u16[dense_n] : nullptr;
    u16 d = 0;
    for (u16 i = 0; i < out.m_res_n; ++i) {
        if (used == nullptr || used[i] == 0) {
            continue;
        }
        out.m_res_map[i] = d;
        out.m_res_ids[d] = i;
        ++d;
    }
    delete[] used;

    out.m_group_n = total;
    out.m_group = (total > 0) ? new DynProduceGroup[total] : nullptr;
    out.m_act = (total > 0) ? new u16[total] : nullptr;
    out.m_touch_res = (out.m_inv_n > 0) ? new u8[out.m_inv_n] : nullptr;
    u16 w = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        DynProduceGroup& g = out.m_group[w++];
        g.m_enabler.m_kind = EffectEnablerKind::BUILDING;
        g.m_enabler.m_idx = i;
        fill_slots(fx, g, out.m_res_map, out.m_res_n);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.tech().get_item(TechStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        DynProduceGroup& g = out.m_group[w++];
        g.m_enabler.m_kind = EffectEnablerKind::TECH;
        g.m_enabler.m_idx = i;
        fill_slots(fx, g, out.m_res_map, out.m_res_n);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        DynProduceGroup& g = out.m_group[w++];
        g.m_enabler.m_kind = EffectEnablerKind::SMALL_WONDER;
        g.m_enabler.m_idx = i;
        fill_slots(fx, g, out.m_res_map, out.m_res_n);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        const ItemEffectsStruct& fx = st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects;
        if (count_produce(fx) == 0) {
            continue;
        }
        DynProduceGroup& g = out.m_group[w++];
        g.m_enabler.m_kind = EffectEnablerKind::WONDER;
        g.m_enabler.m_idx = i;
        fill_slots(fx, g, out.m_res_map, out.m_res_n);
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
