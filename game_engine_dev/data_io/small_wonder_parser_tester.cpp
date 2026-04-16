//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "small_wonder_parser.h"
#include "small_wonder_static_data.h"
#include "data_reader.h"
#include "path_mng.h"
#include "name_to_idx_callbacks.h"
#include "item_effects.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
int print_level = 0;

const DataParserBase* g_tech_parser = NULL;
const DataParserBase* g_resource_parser = NULL;
const DataParserBase* g_city_flag_parser = NULL;
const DataParserBase* g_building_parser = NULL;
const DataParserBase* g_civ_parser = NULL;
const DataParserBase* g_unit_parser = NULL;
const DataParserBase* g_unit_types_parser = NULL;
const DataParserBase* g_civ_traits_parser = NULL;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

u16 cb_tech_name_to_idx (cstr name) {
    return g_tech_parser->name_to_idx(name);
}

u16 cb_resource_name_to_idx (cstr name) {
    return g_resource_parser->name_to_idx(name);
}

u16 cb_city_flag_name_to_idx (cstr name) {
    return g_city_flag_parser->name_to_idx(name);
}

u16 cb_building_name_to_idx (cstr name) {
    return g_building_parser->name_to_idx(name);
}

u16 cb_civ_name_to_idx (cstr name) {
    return g_civ_parser->name_to_idx(name);
}

u16 cb_unit_name_to_idx (cstr name) {
    return g_unit_parser->name_to_idx(name);
}

u16 cb_unit_types_name_to_idx (cstr name) {
    return g_unit_types_parser->name_to_idx(name);
}

u16 cb_civ_trait_name_to_idx (cstr name) {
    return g_civ_traits_parser->name_to_idx(name);
}

std::string req_idx_to_name (u8 req_type, u16 idx) {
    if (req_type == ITEM_REQ_TYPE_TECH && g_tech_parser != NULL) {
        return g_tech_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_RESOURCE && g_resource_parser != NULL) {
        return g_resource_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_FLAG && g_city_flag_parser != NULL) {
        return g_city_flag_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_CIV && g_civ_parser != NULL) {
        return g_civ_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_BUILDING && g_building_parser != NULL) {
        return g_building_parser->idx_to_name(idx);
    }
    return "<unknown>";
}

void print_u16_member (cstr label, u16 value) {
    printf("  %s: %u\n", label, value);
}

void print_u32_member (cstr label, u32 value) {
    printf("  %s: %u\n", label, value);
}

void print_reqs_member (cstr label, const ItemReqsStruct& reqs) {
    printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (reqs.types[j] == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 idx = reqs.indices[j];
        if (idx == 0) {
            continue;
        }
        const u8 type = reqs.types[j];
        const std::string req_name = req_idx_to_name(type, idx);
        printf("    [%u] type=%u %s (%u)", j, type, req_name.c_str(), idx);
        if (reqs.added_args[j] != 0) {
            printf(" arg=%u", reqs.added_args[j]);
        }
        printf("\n");
    }
}

void print_effects_member (cstr label, const ItemEffectsStruct& e) {
    printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        const ItemEffectStruct& slot = e.items[j];
        const u16 type_u = slot.type;
        if (type_u == static_cast<u16>(ItemEffectType::NONE)) {
            continue;
        }
        printf("    [%u] type=%u", j, static_cast<u32>(type_u));
        switch (static_cast<ItemEffectType>(type_u)) {
        case ItemEffectType::BOOSTER: {
            const ItemEffectBooster& b = slot.effect.booster;
            const char* tname = "?";
            switch (b.target_id) {
                case ItemEffectBoosterType::SCIENCE: tname = "SCIENCE"; break;
                case ItemEffectBoosterType::HAPPINESS: tname = "HAPPINESS"; break;
                case ItemEffectBoosterType::PRODUCTION: tname = "PRODUCTION"; break;
                case ItemEffectBoosterType::AIR_DEFENSE: tname = "AIR_DEFENSE"; break;
                case ItemEffectBoosterType::AIR_RANGE: tname = "AIR_RANGE"; break;
                case ItemEffectBoosterType::COMMERCE: tname = "COMMERCE"; break;
                case ItemEffectBoosterType::CORRUPTION: tname = "CORRUPTION"; break;
                case ItemEffectBoosterType::DEFENSE: tname = "DEFENSE"; break;
                case ItemEffectBoosterType::ESPIONAGE: tname = "ESPIONAGE"; break;
                case ItemEffectBoosterType::MOVEMENT: tname = "MOVEMENT"; break;
                case ItemEffectBoosterType::NUKE_DEFENSE: tname = "NUKE_DEFENSE"; break;
                case ItemEffectBoosterType::POLLUTION: tname = "POLLUTION"; break;
                case ItemEffectBoosterType::POP_GROWTH: tname = "POP_GROWTH"; break;
                case ItemEffectBoosterType::SEA_TRADE: tname = "SEA_TRADE"; break;
                case ItemEffectBoosterType::SHIP_DEFENSE: tname = "SHIP_DEFENSE"; break;
                case ItemEffectBoosterType::SHIP_MOVEMENT: tname = "SHIP_MOVEMENT"; break;
                case ItemEffectBoosterType::SHIP_TRAINING: tname = "SHIP_TRAINING"; break;
                case ItemEffectBoosterType::UNIT_EXP: tname = "UNIT_EXP"; break;
                case ItemEffectBoosterType::UPGRADE_COST: tname = "UPGRADE_COST"; break;
                case ItemEffectBoosterType::WAR_WEAR: tname = "WAR_WEAR"; break;
                default: break;
            }
            const char* sc = "?";
            switch (b.scope) {
                case ItemEffectsScope::LOCAL: sc = "LOCAL"; break;
                case ItemEffectsScope::CITY: sc = "CITY"; break;
                case ItemEffectsScope::GLOBAL: sc = "CIV"; break;
                default: break;
            }
            const char* am = "?";
            switch (b.amount_mode) {
                case ItemEffectAmountMode::COUNT: am = "COUNT"; break;
                case ItemEffectAmountMode::PERCENTAGE: am = "PERCENTAGE"; break;
                default: break;
            }
            printf(" booster %s (%d)", tname, static_cast<int>(b.amount));
            printf(" scope=%s", sc);
            printf(" mode=%s", am);
            break;
        }
        case ItemEffectType::BUILD: {
            const ItemEffectBuild& b = slot.effect.build;
            const char* sc = "?";
            switch (b.scope) {
                case ItemEffectsScope::LOCAL: sc = "LOCAL"; break;
                case ItemEffectsScope::CITY: sc = "CITY"; break;
                case ItemEffectsScope::GLOBAL: sc = "CIV"; break;
                default: break;
            }
            const char* bm = "?";
            switch (b.build_mode) {
                case ItemEffectBuildMode::AUTO_BUILD: bm = "AUTO_BUILD"; break;
                case ItemEffectBuildMode::NO_BUILD: bm = "NO_BUILD"; break;
                default: break;
            }
            const char* um = "?";
            switch (b.upkeep_mode) {
                case ItemEffectUpkeepMode::NORMAL_UPKEEP: um = "NORMAL_UPKEEP"; break;
                case ItemEffectUpkeepMode::NO_UPKEEP: um = "NO_UPKEEP"; break;
                default: break;
            }
            if (b.building_id == 0) {
                printf(" build");
            } else {
                const std::string bname = g_building_parser->idx_to_name(b.building_id);
                printf(" build %s (%u)", bname.c_str(), static_cast<u32>(b.building_id));
            }
            printf(" scope=%s", sc);
            printf(" build_mode=%s upkeep=%s", bm, um);
            break;
        }
        case ItemEffectType::ENABLE: {
            const ItemEffectEnable& en = slot.effect.enable;
            const char* fn = "?";
            switch (en.feature_id) {
                case 1: fn = "ALL_GOVERNMENTS"; break;
                case 2: fn = "UNIT_VETERAN"; break;
                case 3: fn = "DIPLOMACY"; break;
                case 4: fn = "NUKES"; break;
                case 5: fn = "SPACE"; break;
                case 6: fn = "SHIP_BUILD"; break;
                case 7: fn = "AIR_UNIT"; break;
                default: break;
            }
            const char* sc = "?";
            switch (en.scope) {
                case ItemEffectsScope::LOCAL: sc = "LOCAL"; break;
                case ItemEffectsScope::CITY: sc = "CITY"; break;
                case ItemEffectsScope::GLOBAL: sc = "CIV"; break;
                default: break;
            }
            printf(" enable %s (%u)", fn, static_cast<u32>(en.feature_id));
            printf(" scope=%s", sc);
            break;
        }
        case ItemEffectType::RESEARCH_TECH: {
            const ItemEffectResearchTech& rt = slot.effect.research_tech;
            printf(" researchTech (%u)", static_cast<u32>(rt.tech_count));
            break;
        }
        case ItemEffectType::TRAIN: {
            const ItemEffectTrain& tr = slot.effect.train;
            if (tr.unit_id == 0) {
                printf(" train");
            } else {
                const std::string uname =
                    (g_unit_parser != NULL) ? g_unit_parser->idx_to_name(tr.unit_id) : std::string("<unknown>");
                printf(" train %s (%u)", uname.c_str(), static_cast<u32>(tr.unit_id));
            }
            if (tr.turns_interval != 0) {
                printf(" interval=%u", static_cast<u32>(tr.turns_interval));
            }
            break;
        }
        case ItemEffectType::TERRAIN_BOOSTER:
            printf(" TERRAIN_BOOSTER");
            break;
        default:
            printf(" <unhandled>");
            break;
        }
        printf("\n");
    }
}

void print_civ_traits_member (cstr label, const CivTraitStruct& traits) {
    printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_CIV_TRAIT_COUNT; ++j) {
        const u16 tix = traits.indices[j];
        if (tix == 0) {
            continue;
        }
        const std::string trait_name = g_civ_traits_parser->idx_to_name(tix);
        printf("    [%u] %s (%u)\n", j, trait_name.c_str(), tix);
    }
}

void print_item (const SmallWonderStaticDataStruct& item) {
    printf("name: %s\n", item.name.c_str());
    print_u32_member("cost", item.cost);
    print_reqs_member("reqs", item.reqs);
    print_effects_member("effects", item.effects);
}

int run_parse_driver () {
    NameToIdxCbs cbs = {};
    cbs.tech_name_to_idx = cb_tech_name_to_idx;
    cbs.resource_name_to_idx = cb_resource_name_to_idx;
    cbs.city_flag_name_to_idx = cb_city_flag_name_to_idx;
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.civ_name_to_idx = cb_civ_name_to_idx;
    cbs.unit_name_to_idx = cb_unit_name_to_idx;
    cbs.unit_type_name_to_idx = cb_unit_types_name_to_idx;
    cbs.civ_trait_name_to_idx = cb_civ_trait_name_to_idx;

    PathMng paths("../");
    DataReader tech_reader(paths.get_path_to_techs());
    DataReader resource_reader(paths.get_path_to_resources());
    DataReader city_flag_reader(paths.get_path_to_city_flags());
    DataReader building_reader(paths.get_path_to_buildings());
    DataReader civ_reader(paths.get_path_to_civs());
    DataReader unit_reader(paths.get_path_to_units());
    DataReader unit_types_reader(paths.get_path_to_unit_types());
    DataReader civ_traits_reader(paths.get_path_to_civ_traits());

    DataReader effect_reader(paths.get_path_to_effects());
    const std::vector<RawItem>& effect_items = effect_reader.get_raw_items();
    DataParserBase::set_item_effect_handler(&cbs, &effect_items);

    DataParserBase tech_parser(tech_reader.get_raw_items(), cbs);
    DataParserBase resource_parser(resource_reader.get_raw_items(), cbs);
    DataParserBase city_flag_parser(city_flag_reader.get_raw_items(), cbs);
    DataParserBase building_parser(building_reader.get_raw_items(), cbs);
    DataParserBase civ_parser(civ_reader.get_raw_items(), cbs);
    DataParserBase unit_parser(unit_reader.get_raw_items(), cbs);
    DataParserBase unit_types_parser(unit_types_reader.get_raw_items(), cbs);
    DataParserBase civ_traits_parser(civ_traits_reader.get_raw_items(), cbs);

    g_tech_parser = &tech_parser;
    g_resource_parser = &resource_parser;
    g_city_flag_parser = &city_flag_parser;
    g_building_parser = &building_parser;
    g_civ_parser = &civ_parser;
    g_unit_parser = &unit_parser;
    g_unit_types_parser = &unit_types_parser;
    g_civ_traits_parser = &civ_traits_parser;

    DataReader reader("../game_config.small_wonders");
    const std::vector<RawItem>& raw_items = reader.get_raw_items();

    SmallWonderParser parser(raw_items, cbs);
    SmallWonderStaticDataStruct* parsed_data = parser.parse_data_dependencies();

    if (print_level >= 2) {
        for (u32 i = 0; i < raw_items.size(); ++i) {
            print_item(parsed_data[i]);
            printf("-----------------------------------------------------------\n");
        }
    }

    delete[] parsed_data;
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    return run_parse_driver();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
