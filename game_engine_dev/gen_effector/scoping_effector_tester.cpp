//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "city_effector.h"
#include "civ_effector.h"
#include "effect_rev_mapper.h"
#include "global_effector.h"
#include "item_effect_helpers.h"
#include "item_effects.h"
#include "item_reqs.h"
#include "local_effector.h"
#include "static_parsing_manager.h"

typedef const char* cstr;

//================================================================================================================================
//=> - Print helpers -
//================================================================================================================================

cstr get_prereq_label (u16 prereq_type) {
    if (prereq_type == ITEM_REQ_TYPE_BUILDING) return "building";
    if (prereq_type == ITEM_REQ_TYPE_TECH) return "tech";
    if (prereq_type == ITEM_REQ_TYPE_BUILDING + 1) return "small_wonder";
    if (prereq_type == ITEM_REQ_TYPE_BUILDING + 2) return "wonder";
    return "unknown";
}

cstr get_prereq_name (const StaticParsingManager& mgr, u16 prereq_type, u16 prereq_idx) {
    if (prereq_type == ITEM_REQ_TYPE_BUILDING) {
        const BuildingStaticDataStruct* a = mgr.get_building_data();
        if (a != nullptr && prereq_idx < mgr.get_building_count()) return a[prereq_idx].name.c_str();
        return "?";
    }
    if (prereq_type == ITEM_REQ_TYPE_TECH) {
        const TechStaticDataStruct* a = mgr.get_tech_data();
        if (a != nullptr && prereq_idx < mgr.get_tech_count()) return a[prereq_idx].name.c_str();
        return "?";
    }
    if (prereq_type == ITEM_REQ_TYPE_BUILDING + 1) {
        const SmallWonderStaticDataStruct* a = mgr.get_small_wonder_data();
        if (a != nullptr && prereq_idx < mgr.get_small_wonder_count()) return a[prereq_idx].name.c_str();
        return "?";
    }
    if (prereq_type == ITEM_REQ_TYPE_BUILDING + 2) {
        const WonderStaticDataStruct* a = mgr.get_wonder_data();
        if (a != nullptr && prereq_idx < mgr.get_wonder_count()) return a[prereq_idx].name.c_str();
        return "?";
    }
    return "?";
}

cstr get_building_name (const StaticParsingManager& mgr, u16 idx) {
    const BuildingStaticDataStruct* a = mgr.get_building_data();
    if (a != nullptr && idx < mgr.get_building_count()) return a[idx].name.c_str();
    return "?";
}

cstr get_unit_name (const StaticParsingManager& mgr, u16 idx) {
    const UnitStaticDataStruct* a = mgr.get_unit_data();
    if (a != nullptr && idx < mgr.get_unit_count()) return a[idx].name.c_str();
    return "?";
}

cstr get_enable_feature_name (u16 feature_id) {
    if (feature_id == 1) return "ALL_GOVERNMENTS";
    if (feature_id == 2) return "UNIT_VETERAN";
    if (feature_id == 3) return "DIPLOMACY";
    if (feature_id == 4) return "NUKES";
    if (feature_id == 5) return "SPACE";
    if (feature_id == 6) return "SHIP_BUILD";
    if (feature_id == 7) return "AIR_UNIT";
    return "?";
}

void build_semantic_effect_text (const StaticParsingManager& mgr, const ItemEffectStruct& fx, char* out, u16 out_sz) {
    if (fx.type == static_cast<u16>(ItemEffectType::NONE)) {
        std::snprintf(out, out_sz, "effect=<none>");
        return;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::BOOSTER)) {
        const ItemEffectBooster& b = fx.effect.booster;
        std::snprintf(out, out_sz, "effect=booster(%s %d %s %s)",
            ItemEffectHelper::booster_type_enum_to_str(b.target_id).c_str(),
            static_cast<int>(b.amount),
            ItemEffectHelper::effects_scope_enum_to_str(b.scope).c_str(),
            ItemEffectHelper::amount_mode_enum_to_str(b.amount_mode).c_str());
        return;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::BUILD)) {
        const ItemEffectBuild& b = fx.effect.build;
        std::snprintf(out, out_sz, "effect=build(%s %s %s %s)",
            get_building_name(mgr, b.building_id),
            ItemEffectHelper::effects_scope_enum_to_str(b.scope).c_str(),
            ItemEffectHelper::build_mode_enum_to_str(b.build_mode).c_str(),
            ItemEffectHelper::upkeep_mode_enum_to_str(b.upkeep_mode).c_str());
        return;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::ENABLE)) {
        const ItemEffectEnable& e = fx.effect.enable;
        std::snprintf(out, out_sz, "effect=enable(%s %s)",
            get_enable_feature_name(e.feature_id),
            ItemEffectHelper::effects_scope_enum_to_str(e.scope).c_str());
        return;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::RESEARCH_TECH)) {
        std::snprintf(out, out_sz, "effect=research_tech(%u)",
            fx.effect.research_tech.tech_count);
        return;
    }
    if (fx.type == static_cast<u16>(ItemEffectType::TRAIN)) {
        const ItemEffectTrain& t = fx.effect.train;
        std::snprintf(out, out_sz, "effect=train(%s every %u turns)",
            get_unit_name(mgr, t.unit_id),
            static_cast<u16>(t.turns_interval));
        return;
    }
    std::snprintf(out, out_sz, "effect=%s",
        ItemEffectHelper::type_enum_to_str(static_cast<ItemEffectType>(fx.type)).c_str());
}

void build_left_text (u16 row_i, u16 prereq_type, u16 prereq_idx, cstr prereq_label, cstr prereq_name, char* out, u16 out_sz) {
    std::snprintf(out, out_sz, "%4u : if %s[%u]=%s[%u]", row_i, prereq_label, prereq_type, prereq_name, prereq_idx);
}

void print_scope_list (const StaticParsingManager& mgr, cstr scope_name, const EffectMapStruct* rows, u16 n) {
    std::printf("\n--- %s SCOPE (%u) ---\n", scope_name, n);
    if (rows == nullptr || n == 0) {
        return;
    }
    u16 left_w = 0;
    for (u16 i = 0; i < n; ++i) {
        char left[256];
        build_left_text(i, rows[i].prereq_type, rows[i].prereq_idx,
            get_prereq_label(rows[i].prereq_type),
            get_prereq_name(mgr, rows[i].prereq_type, rows[i].prereq_idx),
            left, sizeof(left));
        const u16 left_n = static_cast<u16>(std::strlen(left));
        if (left_n > left_w) left_w = left_n;
    }
    for (u16 i = 0; i < n; ++i) {
        char left[256];
        char effect[384];
        build_left_text(i, rows[i].prereq_type, rows[i].prereq_idx,
            get_prereq_label(rows[i].prereq_type),
            get_prereq_name(mgr, rows[i].prereq_type, rows[i].prereq_idx),
            left, sizeof(left));
        build_semantic_effect_text(mgr, rows[i].effect, effect, sizeof(effect));
        std::printf("%s", left);
        const u16 left_n = static_cast<u16>(std::strlen(left));
        for (u16 p = left_n; p < left_w + 2; ++p) std::printf(" ");
        std::printf("-> %s\n", effect);
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    StaticParsingManager mgr("../");
    u16 flat_n = 0;
    EffectMapStruct* flat = EffectRevMapper::build_flat_list(mgr, &flat_n);

    GlobalEffector global_fx;
    LocalEffector local_fx;
    CityEffector city_fx;
    CivEffector civ_fx;
    global_fx.parse(flat, flat_n);
    local_fx.parse(flat, flat_n);
    city_fx.parse(flat, flat_n);
    civ_fx.parse(flat, flat_n);

    const u16 global_n = global_fx.get_count();
    const u16 local_n = local_fx.get_count();
    const u16 city_n = city_fx.get_count();
    const u16 civ_n = civ_fx.get_count();
    const u16 scoped_n = global_n + local_n + city_n + civ_n;
    const u16 unscoped_n = flat_n - scoped_n;

    std::printf("FLAT EFFECT COUNT: %u\n", flat_n);
    print_scope_list(mgr, "GLOBAL", global_fx.get_rows(), global_n);
    print_scope_list(mgr, "LOCAL", local_fx.get_rows(), local_n);
    print_scope_list(mgr, "CITY", city_fx.get_rows(), city_n);
    print_scope_list(mgr, "CIV", civ_fx.get_rows(), civ_n);

    std::printf("\n--- SUMMARY ---\n");
    std::printf(" total flat:     %u\n", flat_n);
    std::printf(" global scope:   %u\n", global_n);
    std::printf(" local scope:    %u\n", local_n);
    std::printf(" city scope:     %u\n", city_n);
    std::printf(" civ scope:      %u\n", civ_n);
    std::printf(" lacks scope:    %u  (total - global - local - city - civ)\n", unscoped_n);

    EffectRevMapper::release_flat_list(flat);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
