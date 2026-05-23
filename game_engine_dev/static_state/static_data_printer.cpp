//================================================================================================================================
//=> - WARNING -
//================================================================================================================================
//
//  - Template for static_data_printer (filled by generate_static_data_and_test.py)
//  - Do not edit generated output manually.
//
//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "static_data_printer.h"
#include "runtime_statics.h"
#include "item_reqs.h"
#include "item_effects.h"

#include "building_static_data.h"
#include "city_flag_static_data.h"
#include "civ_static_data.h"
#include "civ_trait_static_data.h"
#include "resource_static_data.h"
#include "small_wonder_static_data.h"
#include "tech_static_data.h"
#include "unit_static_data.h"
#include "unit_action_static_data.h"
#include "unit_type_static_data.h"
#include "wonder_static_data.h"

//================================================================================================================================
//=> - StaticDataPrinter helpers -
//================================================================================================================================

void StaticDataPrinter::print_indent (i32 print_lvl) {
    for (i32 i = 0; i < print_lvl * 2; ++i) {
        printf(" ");
    }
}

bool StaticDataPrinter::can_recurse (i32 print_lvl, i32 max_lvl) {
    return print_lvl < max_lvl;
}

void StaticDataPrinter::print_u16 (const char* label, u16 value, i32 print_lvl) {
    print_indent(print_lvl);
    printf("%s: %u\n", label, value);
}

void StaticDataPrinter::print_u32 (const char* label, u32 value, i32 print_lvl) {
    print_indent(print_lvl);
    printf("%s: %u\n", label, static_cast<u32>(value));
}

//================================================================================================================================
//=> - Per-kind item printers -
//================================================================================================================================

static void print_building_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const BuildingStaticDataStruct& item = statics.building().get_item(BuildingStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("building[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_u32("cost", item.cost, print_lvl + 1);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("effects:\n");
    for (u32 ej = 0; ej < MAX_EFFECT_COUNT; ++ej) {
        const ItemEffectStruct& slot = item.effects.items[ej];
        if (slot.type == 0) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u\n", ej, slot.type);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (slot.type == static_cast<u16>(ItemEffectType::BUILD)) {
            u16 ref_idx = slot.effect.build.building_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ref_idx, print_lvl + 1, max_lvl);
            }
        }
        if (slot.type == static_cast<u16>(ItemEffectType::TRAIN)) {
            u16 ref_idx = slot.effect.train.unit_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::UNIT, ref_idx, print_lvl + 1, max_lvl);
            }
        }
    }
}

static void print_city_flag_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const CityFlagStaticDataStruct& item = statics.city_flag().get_item(CityFlagStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("city_flag[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("effects:\n");
    for (u32 ej = 0; ej < MAX_EFFECT_COUNT; ++ej) {
        const ItemEffectStruct& slot = item.effects.items[ej];
        if (slot.type == 0) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u\n", ej, slot.type);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (slot.type == static_cast<u16>(ItemEffectType::BUILD)) {
            u16 ref_idx = slot.effect.build.building_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ref_idx, print_lvl + 1, max_lvl);
            }
        }
        if (slot.type == static_cast<u16>(ItemEffectType::TRAIN)) {
            u16 ref_idx = slot.effect.train.unit_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::UNIT, ref_idx, print_lvl + 1, max_lvl);
            }
        }
    }
}

static void print_civ_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const CivStaticDataStruct& item = statics.civ().get_item(CivStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("civ[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("traits:\n");
    for (u8 ti = 0; ti < MAX_CIV_TRAIT_COUNT; ++ti) {
        u16 tix = item.traits.indices[ti];
        if (tix == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] idx=%u\n", ti, tix);
        if (StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV_TRAIT, tix, print_lvl + 1, max_lvl);
        }
    }
}

static void print_civ_trait_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const CivTraitStaticDataStruct& item = statics.civ_trait().get_item(CivTraitStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("civ_trait[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
}

static void print_resource_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const ResourceStaticDataStruct& item = statics.resource().get_item(ResourceStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("resource[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_u16("food", static_cast<u16>(item.food), print_lvl + 1);
    StaticDataPrinter::print_u16("shields", static_cast<u16>(item.shields), print_lvl + 1);
    StaticDataPrinter::print_u16("commerce", static_cast<u16>(item.commerce), print_lvl + 1);
}

static void print_small_wonder_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const SmallWonderStaticDataStruct& item = statics.small_wonder().get_item(SmallWonderStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("small_wonder[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_u32("cost", item.cost, print_lvl + 1);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("effects:\n");
    for (u32 ej = 0; ej < MAX_EFFECT_COUNT; ++ej) {
        const ItemEffectStruct& slot = item.effects.items[ej];
        if (slot.type == 0) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u\n", ej, slot.type);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (slot.type == static_cast<u16>(ItemEffectType::BUILD)) {
            u16 ref_idx = slot.effect.build.building_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ref_idx, print_lvl + 1, max_lvl);
            }
        }
        if (slot.type == static_cast<u16>(ItemEffectType::TRAIN)) {
            u16 ref_idx = slot.effect.train.unit_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::UNIT, ref_idx, print_lvl + 1, max_lvl);
            }
        }
    }
}

static void print_tech_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const TechStaticDataStruct& item = statics.tech().get_item(TechStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("tech[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("effects:\n");
    for (u32 ej = 0; ej < MAX_EFFECT_COUNT; ++ej) {
        const ItemEffectStruct& slot = item.effects.items[ej];
        if (slot.type == 0) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u\n", ej, slot.type);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (slot.type == static_cast<u16>(ItemEffectType::BUILD)) {
            u16 ref_idx = slot.effect.build.building_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ref_idx, print_lvl + 1, max_lvl);
            }
        }
        if (slot.type == static_cast<u16>(ItemEffectType::TRAIN)) {
            u16 ref_idx = slot.effect.train.unit_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::UNIT, ref_idx, print_lvl + 1, max_lvl);
            }
        }
    }
    StaticDataPrinter::print_u32("cost", item.cost, print_lvl + 1);
    StaticDataPrinter::print_u16("tier", static_cast<u16>(item.tier), print_lvl + 1);
}

static void print_unit_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const UnitStaticDataStruct& item = statics.unit().get_item(UnitStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("unit[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_u32("cost", item.cost, print_lvl + 1);
    StaticDataPrinter::print_u16("type", static_cast<u16>(item.type), print_lvl + 1);
    StaticDataPrinter::print_u16("attack", static_cast<u16>(item.attack), print_lvl + 1);
    StaticDataPrinter::print_u16("defense", static_cast<u16>(item.defense), print_lvl + 1);
    StaticDataPrinter::print_u16("mvt_pts", static_cast<u16>(item.mvt_pts), print_lvl + 1);
}

static void print_unit_action_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const UnitActionStaticDataStruct& item = statics.unit_action().get_item(UnitActionStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("unit_action[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
}

static void print_unit_type_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const UnitTypeStaticDataStruct& item = statics.unit_type().get_item(UnitTypeStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("unit_type[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
}

static void print_wonder_item (
    const RuntimeStatics& statics,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    const WonderStaticDataStruct& item = statics.wonder().get_item(WonderStaticDataKey::from_raw(idx));
    StaticDataPrinter::print_indent(print_lvl);
    printf("wonder[%u]\n", idx);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("name: %s\n", item.name.c_str());
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("reqs:\n");
    for (u8 ri = 0; ri < MAX_PREREQ_COUNT; ++ri) {
        u8 rt = item.reqs.types[ri];
        u16 ridx = item.reqs.indices[ri];
        if (ridx == U16_KEY_NULL) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u idx=%u\n", ri, rt, ridx);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (rt == ITEM_REQ_TYPE_TECH) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::TECH, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_RESOURCE) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::RESOURCE, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_FLAG) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CITY_FLAG, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_CIV) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::CIV, ridx, print_lvl + 1, max_lvl);
        }
        if (rt == ITEM_REQ_TYPE_BUILDING) {
            StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ridx, print_lvl + 1, max_lvl);
        }
    }
    StaticDataPrinter::print_u32("cost", item.cost, print_lvl + 1);
    StaticDataPrinter::print_indent(print_lvl + 1);
    printf("effects:\n");
    for (u32 ej = 0; ej < MAX_EFFECT_COUNT; ++ej) {
        const ItemEffectStruct& slot = item.effects.items[ej];
        if (slot.type == 0) {
            continue;
        }
        StaticDataPrinter::print_indent(print_lvl + 1 + 1);
        printf("[%u] type=%u\n", ej, slot.type);
        if (!StaticDataPrinter::can_recurse(print_lvl, max_lvl)) {
            continue;
        }
        if (slot.type == static_cast<u16>(ItemEffectType::BUILD)) {
            u16 ref_idx = slot.effect.build.building_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::BUILDING, ref_idx, print_lvl + 1, max_lvl);
            }
        }
        if (slot.type == static_cast<u16>(ItemEffectType::TRAIN)) {
            u16 ref_idx = slot.effect.train.unit_id;
            if (ref_idx != U16_KEY_NULL) {
                StaticDataPrinter::print_selected_items(statics, StaticDataPrintKind::UNIT, ref_idx, print_lvl + 1, max_lvl);
            }
        }
    }
}


//================================================================================================================================
//=> - Dispatch -
//================================================================================================================================

void StaticDataPrinter::print_selected_items (
    const RuntimeStatics& statics,
    StaticDataPrintKind kind,
    u16 idx,
    i32 print_lvl,
    i32 max_lvl) {

    if (print_lvl > max_lvl) {
        return;
    }
    switch (kind) {
    case StaticDataPrintKind::BUILDING:
        print_building_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::CITY_FLAG:
        print_city_flag_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::CIV:
        print_civ_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::CIV_TRAIT:
        print_civ_trait_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::RESOURCE:
        print_resource_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::SMALL_WONDER:
        print_small_wonder_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::TECH:
        print_tech_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::UNIT:
        print_unit_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::UNIT_ACTION:
        print_unit_action_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::UNIT_TYPE:
        print_unit_type_item(statics, idx, print_lvl, max_lvl);
        break;
    case StaticDataPrintKind::WONDER:
        print_wonder_item(statics, idx, print_lvl, max_lvl);
        break;
    default:
        print_indent(print_lvl);
        printf("<unknown kind %u> idx=%u\n", static_cast<u32>(kind), idx);
        break;
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
