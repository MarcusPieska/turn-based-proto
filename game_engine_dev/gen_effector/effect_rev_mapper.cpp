//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "effect_rev_mapper.h"

#include "item_reqs.h"
#include "static_parsing_manager.h"

//================================================================================================================================
//=> - EffectRevMapper helpers -
//================================================================================================================================

bool EffectRevMapper::is_effect_set (const ItemEffectStruct& fx) {
    return fx.type != static_cast<u16>(ItemEffectType::NONE);
}

static u16 src_building () {
    return ITEM_REQ_TYPE_BUILDING;
}

static u16 src_small_wonder () {
    return static_cast<u16>(ITEM_REQ_TYPE_BUILDING + 1);
}

static u16 src_tech () {
    return ITEM_REQ_TYPE_TECH;
}

static u16 src_wonder () {
    return static_cast<u16>(ITEM_REQ_TYPE_BUILDING + 2);
}

static void set_row (EffectMapStruct& row, u16 tp, u16 idx, const ItemEffectStruct& fx) {
    row.prereq_type = tp;
    row.prereq_idx = idx;
    row.effect = fx;
}

static u16 count_item_effects (const ItemEffectsStruct& effects) {
    u16 n = 0;
    for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        if (effects.items[j].type != static_cast<u16>(ItemEffectType::NONE)) {
            ++n;
        }
    }
    return n;
}

//================================================================================================================================
//=> - EffectRevMapper factory -
//================================================================================================================================

EffectMapStruct* EffectRevMapper::build_flat_list (const StaticParsingManager& mgr, u16* out_count) {
    if (out_count == nullptr) {
        return nullptr;
    }
    *out_count = 0;

    const BuildingStaticDataStruct* b = mgr.get_building_data();
    const SmallWonderStaticDataStruct* sw = mgr.get_small_wonder_data();
    const TechStaticDataStruct* t = mgr.get_tech_data();
    const WonderStaticDataStruct* w = mgr.get_wonder_data();
    const u16 b_n = mgr.get_building_count();
    const u16 sw_n = mgr.get_small_wonder_count();
    const u16 t_n = mgr.get_tech_count();
    const u16 w_n = mgr.get_wonder_count();

    u16 total = 0;
    for (u16 i = 0; i < b_n; ++i) total += count_item_effects(b[i].effects);
    for (u16 i = 0; i < sw_n; ++i) total += count_item_effects(sw[i].effects);
    for (u16 i = 0; i < t_n; ++i) total += count_item_effects(t[i].effects);
    for (u16 i = 0; i < w_n; ++i) total += count_item_effects(w[i].effects);

    if (total == 0) {
        return nullptr;
    }

    EffectMapStruct* rows = new EffectMapStruct[total];
    u16 k = 0;
    for (u16 i = 0; i < b_n; ++i) {
        for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
            const ItemEffectStruct& fx = b[i].effects.items[j];
            if (!EffectRevMapper::is_effect_set(fx)) continue;
            set_row(rows[k], src_building(), i, fx);
            ++k;
        }
    }
    for (u16 i = 0; i < sw_n; ++i) {
        for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
            const ItemEffectStruct& fx = sw[i].effects.items[j];
            if (!EffectRevMapper::is_effect_set(fx)) continue;
            set_row(rows[k], src_small_wonder(), i, fx);
            ++k;
        }
    }
    for (u16 i = 0; i < t_n; ++i) {
        for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
            const ItemEffectStruct& fx = t[i].effects.items[j];
            if (!EffectRevMapper::is_effect_set(fx)) continue;
            set_row(rows[k], src_tech(), i, fx);
            ++k;
        }
    }
    for (u16 i = 0; i < w_n; ++i) {
        for (u16 j = 0; j < MAX_EFFECT_COUNT; ++j) {
            const ItemEffectStruct& fx = w[i].effects.items[j];
            if (!EffectRevMapper::is_effect_set(fx)) continue;
            set_row(rows[k], src_wonder(), i, fx);
            ++k;
        }
    }

    *out_count = total;
    return rows;
}

void EffectRevMapper::release_flat_list (EffectMapStruct* rows) {
    delete[] rows;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
