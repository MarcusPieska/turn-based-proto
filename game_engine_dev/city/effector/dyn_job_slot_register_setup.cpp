//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "dyn_job_slot_register_setup.h"

#include "building_static_key.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static void tally_fx (const ItemEffectsStruct& fx, u16* counts, u16 job_n, u16& total) {
    const u16 want = static_cast<u16>(ItemEffectType::JOB_SLOTS);
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const u16 job_id = fx.items[j].effect.job_slots.job_id;
        if (job_id >= job_n) {
            continue;
        }
        ++counts[job_id];
        ++total;
    }
}

static void fill_fx (const ItemEffectsStruct& fx, EffectEnablerKind kind, u16 host_idx,
    DynJobSlotEntry* entry, u16* curs, u16 job_n) {
    const u16 want = static_cast<u16>(ItemEffectType::JOB_SLOTS);
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (fx.items[j].type != want) {
            continue;
        }
        const ItemEffectJobSlots& js = fx.items[j].effect.job_slots;
        if (js.job_id >= job_n) {
            continue;
        }
        DynJobSlotEntry& e = entry[curs[js.job_id]++];
        e.m_enabler.m_kind = kind;
        e.m_enabler.m_idx = host_idx;
        e.m_job_id = js.job_id;
        e.m_amount = js.amount;
        e.m_scope = js.scope;
        e.m_mode = js.amount_mode;
    }
}

//================================================================================================================================
//=> - DynJobSlotRegisterSetup -
//================================================================================================================================

bool DynJobSlotRegisterSetup::build (const RuntimeStatics& st, DynJobSlotRegister& out) {
    out.clear();
    const u16 job_n = st.city_job().get_item_count();
    if (job_n == 0) {
        return false;
    }
    u16* counts = new u16[job_n]();
    u16 total = 0;
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        tally_fx(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects, counts, job_n, total);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        tally_fx(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects, counts, job_n, total);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        tally_fx(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects, counts, job_n, total);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        tally_fx(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects, counts, job_n, total);
    }
    out.m_job_n = job_n;
    out.m_entry_n = total;
    out.m_off = new u16[job_n + 1u];
    out.m_off[0] = 0;
    for (u16 i = 0; i < job_n; ++i) {
        out.m_off[i + 1u] = static_cast<u16>(out.m_off[i] + counts[i]);
    }
    out.m_entry = (total > 0) ? new DynJobSlotEntry[total] : nullptr;
    u16* curs = new u16[job_n];
    for (u16 i = 0; i < job_n; ++i) {
        curs[i] = out.m_off[i];
    }
    for (u16 i = 0; i < st.building().get_item_count(); ++i) {
        fill_fx(st.building().get_item(BuildingStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::BUILDING, i, out.m_entry, curs, job_n);
    }
    for (u16 i = 0; i < st.tech().get_item_count(); ++i) {
        fill_fx(st.tech().get_item(TechStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::TECH, i, out.m_entry, curs, job_n);
    }
    for (u16 i = 0; i < st.small_wonder().get_item_count(); ++i) {
        fill_fx(st.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::SMALL_WONDER, i, out.m_entry, curs, job_n);
    }
    for (u16 i = 0; i < st.wonder().get_item_count(); ++i) {
        fill_fx(st.wonder().get_item(WonderStaticDataKey::from_raw(i)).effects,
            EffectEnablerKind::WONDER, i, out.m_entry, curs, job_n);
    }
    delete[] curs;
    delete[] counts;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
