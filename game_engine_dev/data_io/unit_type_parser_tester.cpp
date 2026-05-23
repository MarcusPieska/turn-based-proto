//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "unit_type_parser_tester.h"

//================================================================================================================================
//=> - Static members -
//================================================================================================================================

UnitTypeParserTester* UnitTypeParserTester::s_inst = NULL;

//================================================================================================================================
//=> - UnitTypeParserTester implementation -
//================================================================================================================================

UnitTypeParserTester::UnitTypeParserTester () : 
    m_plvl(0), 
    m_building_psr(NULL),
    m_city_flag_psr(NULL),
    m_civ_psr(NULL),
    m_civ_trait_psr(NULL),
    m_resource_psr(NULL),
    m_small_wonder_psr(NULL),
    m_tech_psr(NULL),
    m_unit_psr(NULL),
    m_unit_action_psr(NULL),
    m_unit_type_psr(NULL),
    m_wonder_psr(NULL) 
{
}

void UnitTypeParserTester::set_plvl (int lvl) {
    m_plvl = lvl;
}

bool UnitTypeParserTester::ld_sm (StringManager& sm, cstr path) {
    if (sm.load_file_content(path)) {
        sm.split_string_by_char(0, '\n');
        sm.cull_empty_strings();
        return true;
    }
    sm.load_cstr_content("NONE");
    sm.split_string_by_char(0, '\n');
    sm.cull_empty_strings();
    return false;
}

u16 UnitTypeParserTester::st_building_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_building_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_building_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_city_flag_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_city_flag_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_city_flag_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_civ_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_civ_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_civ_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_civ_trait_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_civ_trait_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_civ_trait_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_resource_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_resource_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_resource_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_small_wonder_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_small_wonder_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_small_wonder_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_tech_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_tech_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_tech_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_unit_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_unit_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_unit_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_unit_action_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_unit_action_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_unit_action_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_unit_type_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_unit_type_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_unit_type_psr->name_to_idx(name);
}

u16 UnitTypeParserTester::st_wonder_n2i (cstr name) {
    if (s_inst == NULL || s_inst->m_wonder_psr == NULL) {
        return U16_KEY_NULL;
    }
    return s_inst->m_wonder_psr->name_to_idx(name);
}

void UnitTypeParserTester::pr_u16 (cstr label, u16 value) {
    printf("  %s: %u\n", label, value);
}

void UnitTypeParserTester::pr_u32 (cstr label, u32 value) {
    printf("  %s: %u\n", label, value);
}

void UnitTypeParserTester::pr_reqs (cstr label, const ItemReqsStruct& reqs) {
    printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (reqs.types[j] == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 idx = reqs.indices[j];
        if (idx == U16_KEY_NULL) {
            continue;
        }
        const u8 type = reqs.types[j];

        if (type == ITEM_REQ_TYPE_BUILDING && m_building_psr != NULL) {
            printf("    [%u] type=%u %s (%u)", j, type, m_building_psr->idx_to_name(idx).c_str(), idx);
        } else if (type == ITEM_REQ_TYPE_FLAG && m_city_flag_psr != NULL) {
            printf("    [%u] type=%u %s (%u)", j, type, m_city_flag_psr->idx_to_name(idx).c_str(), idx);
        } else if (type == ITEM_REQ_TYPE_CIV && m_civ_psr != NULL) {
            printf("    [%u] type=%u %s (%u)", j, type, m_civ_psr->idx_to_name(idx).c_str(), idx);
        } else if (type == ITEM_REQ_TYPE_RESOURCE && m_resource_psr != NULL) {
            printf("    [%u] type=%u %s (%u)", j, type, m_resource_psr->idx_to_name(idx).c_str(), idx);
        } else if (type == ITEM_REQ_TYPE_TECH && m_tech_psr != NULL) {
            printf("    [%u] type=%u %s (%u)", j, type, m_tech_psr->idx_to_name(idx).c_str(), idx);

        } else {
            printf("    [%u] type=%u <unknown> (%u)", j, type, idx);
        }
        if (reqs.added_args[j] != 0) {
            printf(" arg=%u", reqs.added_args[j]);
        }
        printf("\n");
    }
}

void UnitTypeParserTester::pr_fx (cstr label, const ItemEffectsStruct& e) {
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
            if (b.building_id == U16_KEY_NULL) {
                printf(" build");
            } else if (m_building_psr != NULL) {
                printf(" build %s (%u)", m_building_psr->idx_to_name(b.building_id).c_str(), static_cast<u32>(b.building_id));
            } else {
                printf(" build <unknown> (%u)", static_cast<u32>(b.building_id));
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
            if (tr.unit_id == U16_KEY_NULL) {
                printf(" train");
            } else {
                if (m_unit_psr != NULL) {
                    printf(" train %s (%u)", m_unit_psr->idx_to_name(tr.unit_id).c_str(), static_cast<u32>(tr.unit_id));
                } else {
                    printf(" train <unknown> (%u)", static_cast<u32>(tr.unit_id));
                }
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

void UnitTypeParserTester::pr_traits (cstr label, const CivTraitStruct& traits) {
    printf("  %s:\n", label);
    for (u32 j = 0; j < MAX_CIV_TRAIT_COUNT; ++j) {
        const u16 tix = traits.indices[j];
        if (tix == U16_KEY_NULL) {
            continue;
        }
        if (m_civ_trait_psr != NULL) {
            printf("    [%u] %s (%u)\n", j, m_civ_trait_psr->idx_to_name(tix).c_str(), tix);
        } else {
            printf("    [%u] <unknown> (%u)\n", j, tix);
        }
    }
}

void UnitTypeParserTester::pr_item (const UnitTypeStaticDataStruct& item) {
    printf("name: %s\n", item.name.c_str());
    // No parsing instructions provided
}

int UnitTypeParserTester::run () {
    s_inst = this;
    NameToIdxCbs cbs = {};

    cbs.building_name_to_idx = st_building_n2i;
    cbs.city_flag_name_to_idx = st_city_flag_n2i;
    cbs.civ_name_to_idx = st_civ_n2i;
    cbs.civ_trait_name_to_idx = st_civ_trait_n2i;
    cbs.resource_name_to_idx = st_resource_n2i;
    cbs.small_wonder_name_to_idx = st_small_wonder_n2i;
    cbs.tech_name_to_idx = st_tech_n2i;
    cbs.unit_name_to_idx = st_unit_n2i;
    cbs.unit_action_name_to_idx = st_unit_action_n2i;
    cbs.unit_type_name_to_idx = st_unit_type_n2i;
    cbs.wonder_name_to_idx = st_wonder_n2i;

    PathMng paths("../");

    StringManager building_items;
    StringManager city_flag_items;
    StringManager civ_items;
    StringManager civ_trait_items;
    StringManager resource_items;
    StringManager small_wonder_items;
    StringManager tech_items;
    StringManager unit_items;
    StringManager unit_action_items;
    StringManager unit_type_items;
    StringManager wonder_items;

    StringManager effect_items;

    ld_sm(building_items, paths.get_path_to_buildings().c_str());
    ld_sm(city_flag_items, paths.get_path_to_city_flags().c_str());
    ld_sm(civ_items, paths.get_path_to_civs().c_str());
    ld_sm(civ_trait_items, paths.get_path_to_civ_traits().c_str());
    ld_sm(resource_items, paths.get_path_to_resources().c_str());
    ld_sm(small_wonder_items, paths.get_path_to_small_wonders().c_str());
    ld_sm(tech_items, paths.get_path_to_techs().c_str());
    ld_sm(unit_items, paths.get_path_to_units().c_str());
    ld_sm(unit_action_items, paths.get_path_to_unit_actions().c_str());
    ld_sm(unit_type_items, paths.get_path_to_unit_types().c_str());
    ld_sm(wonder_items, paths.get_path_to_wonders().c_str());

    const bool fx_ok = ld_sm(effect_items, paths.get_path_to_effects().c_str());
    if (fx_ok) {
        DataParserBase::set_item_effect_handler(&cbs, &effect_items);
    }

    DataParserBase building_parser(building_items, cbs);
    DataParserBase city_flag_parser(city_flag_items, cbs);
    DataParserBase civ_parser(civ_items, cbs);
    DataParserBase civ_trait_parser(civ_trait_items, cbs);
    DataParserBase resource_parser(resource_items, cbs);
    DataParserBase small_wonder_parser(small_wonder_items, cbs);
    DataParserBase tech_parser(tech_items, cbs);
    DataParserBase unit_parser(unit_items, cbs);
    DataParserBase unit_action_parser(unit_action_items, cbs);
    DataParserBase unit_type_parser(unit_type_items, cbs);
    DataParserBase wonder_parser(wonder_items, cbs);

    m_building_psr = &building_parser;
    m_city_flag_psr = &city_flag_parser;
    m_civ_psr = &civ_parser;
    m_civ_trait_psr = &civ_trait_parser;
    m_resource_psr = &resource_parser;
    m_small_wonder_psr = &small_wonder_parser;
    m_tech_psr = &tech_parser;
    m_unit_psr = &unit_parser;
    m_unit_action_psr = &unit_action_parser;
    m_unit_type_psr = &unit_type_parser;
    m_wonder_psr = &wonder_parser;

    StringManager raw_items;
    if (!raw_items.load_file_content("../game_config.unit_types")) {
        s_inst = NULL;
        return 0;
    }
    raw_items.split_string_by_char(0, '\n');
    raw_items.cull_empty_strings();
    if (raw_items.get_string_count() == 0) {
        s_inst = NULL;
        return 0;
    }
    UnitTypeParser parser(raw_items, cbs);
    UnitTypeStaticDataStruct* parsed_data = parser.parse_data_dependencies();
    if (m_plvl >= 2) {
        for (u32 i = 0; i < raw_items.get_string_count(); ++i) {
            pr_item(parsed_data[i]);
            printf("-----------------------------------------------------------\n");
        }
    }
    delete[] parsed_data;
    s_inst = NULL;
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
