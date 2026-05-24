//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "[FILE_TAG]_parser_tester.h"

//================================================================================================================================
//=> - Static members -
//================================================================================================================================

[CLASS_TAG]ParserTester* [CLASS_TAG]ParserTester::s_inst = NULL;

//================================================================================================================================
//=> - [CLASS_TAG]ParserTester implementation -
//================================================================================================================================

[CLASS_TAG]ParserTester::[CLASS_TAG]ParserTester () : 
    m_plvl(0), 
    m_out(NULL), 
    [DEP_SD_INIT_TAG], 
    [DEP_INIT_TAG] 
{
}

[DEP_SD_SETTERS_IMPL_TAG]

FILE* [CLASS_TAG]ParserTester::out () const {
    return m_out != NULL ? m_out : stdout;
}

void [CLASS_TAG]ParserTester::open_writer () {
    if (m_out != NULL) {
        return;
    }
    m_out = fopen("RESULTS_NEW_[MACRO_TAG]", "w");
}

void [CLASS_TAG]ParserTester::close_writer () {
    if (m_out != NULL) {
        fclose(m_out);
        m_out = NULL;
    }
}

void [CLASS_TAG]ParserTester::set_plvl (int lvl) {
    m_plvl = lvl;
}

bool [CLASS_TAG]ParserTester::ld_sm (StringManager& sm, cstr path) {
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

[DEP_N2I_IMPL_TAG]
void [CLASS_TAG]ParserTester::pr_u16 (cstr label, u16 value) {
    fprintf(out(), "  %s: %u\n", label, value);
}

void [CLASS_TAG]ParserTester::pr_u32 (cstr label, u32 value) {
    fprintf(out(), "  %s: %u\n", label, value);
}

void [CLASS_TAG]ParserTester::pr_reqs (cstr label, const ItemReqsStruct& reqs) {
    fprintf(out(), "  %s:\n", label);
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (reqs.types[j] == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 idx = reqs.indices[j];
        if (idx == U16_KEY_NULL) {
            continue;
        }
        const u8 type = reqs.types[j];

        [DEP_REQS_PRINT_TAG]

        } else {
            fprintf(out(), "    [%u] type=%u <unknown> (%u)", j, type, idx);
        }
        if (reqs.added_args[j] != 0) {
            fprintf(out(), " arg=%u", reqs.added_args[j]);
        }
        fprintf(out(), "\n");
    }
}

void [CLASS_TAG]ParserTester::pr_fx (cstr label, const ItemEffectsStruct& e) {
    fprintf(out(), "  %s:\n", label);
    for (u32 j = 0; j < MAX_EFFECT_COUNT; ++j) {
        const ItemEffectStruct& slot = e.items[j];
        const u16 type_u = slot.type;
        if (type_u == static_cast<u16>(ItemEffectType::NONE)) {
            continue;
        }
        fprintf(out(), "    [%u] type=%u", j, static_cast<u32>(type_u));
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
            fprintf(out(), " booster %s (%d)", tname, static_cast<int>(b.amount));
            fprintf(out(), " scope=%s", sc);
            fprintf(out(), " mode=%s", am);
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
                fprintf(out(), " build");
            } else if (m_building_sd != NULL && b.building_id < m_building_sd->get_item_count()) {
                fprintf(out(), " build %s (%u)", m_building_sd->get_item(BuildingStaticDataKey::from_raw(b.building_id)).name.c_str(), static_cast<u32>(b.building_id));
            } else if (m_building_psr != NULL) {
                fprintf(out(), " build %s (%u)", m_building_psr->idx_to_name(b.building_id).c_str(), static_cast<u32>(b.building_id));
            } else {
                fprintf(out(), " build <unknown> (%u)", static_cast<u32>(b.building_id));
            }
            fprintf(out(), " scope=%s", sc);
            fprintf(out(), " build_mode=%s upkeep=%s", bm, um);
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
            fprintf(out(), " enable %s (%u)", fn, static_cast<u32>(en.feature_id));
            fprintf(out(), " scope=%s", sc);
            break;
        }
        case ItemEffectType::RESEARCH_TECH: {
            const ItemEffectResearchTech& rt = slot.effect.research_tech;
            fprintf(out(), " researchTech (%u)", static_cast<u32>(rt.tech_count));
            break;
        }
        case ItemEffectType::TRAIN: {
            const ItemEffectTrain& tr = slot.effect.train;
            if (tr.unit_id == U16_KEY_NULL) {
                fprintf(out(), " train");
            } else if (m_unit_sd != NULL && tr.unit_id < m_unit_sd->get_item_count()) {
                fprintf(out(), " train %s (%u)", m_unit_sd->get_item(UnitStaticDataKey::from_raw(tr.unit_id)).name.c_str(), static_cast<u32>(tr.unit_id));
            } else if (m_unit_psr != NULL) {
                fprintf(out(), " train %s (%u)", m_unit_psr->idx_to_name(tr.unit_id).c_str(), static_cast<u32>(tr.unit_id));
            } else {
                fprintf(out(), " train <unknown> (%u)", static_cast<u32>(tr.unit_id));
            }
            if (tr.turns_interval != 0) {
                fprintf(out(), " interval=%u", static_cast<u32>(tr.turns_interval));
            }
            break;
        }
        case ItemEffectType::TERRAIN_BOOSTER:
            fprintf(out(), " TERRAIN_BOOSTER");
            break;
        default:
            fprintf(out(), " <unhandled>");
            break;
        }
        fprintf(out(), "\n");
    }
}

void [CLASS_TAG]ParserTester::pr_traits (cstr label, const CivTraitStruct& traits) {
    fprintf(out(), "  %s:\n", label);
    for (u32 j = 0; j < MAX_CIV_TRAIT_COUNT; ++j) {
        const u16 tix = traits.indices[j];
        if (tix == U16_KEY_NULL) {
            continue;
        }
        if (m_civ_trait_sd != NULL && tix < m_civ_trait_sd->get_item_count()) {
            fprintf(out(), "    [%u] %s (%u)\n", j, m_civ_trait_sd->get_item(CivTraitStaticDataKey::from_raw(tix)).name.c_str(), tix);
        } else if (m_civ_trait_psr != NULL) {
            fprintf(out(), "    [%u] %s (%u)\n", j, m_civ_trait_psr->idx_to_name(tix).c_str(), tix);
        } else {
            fprintf(out(), "    [%u] <unknown> (%u)\n", j, tix);
        }
    }
}

void [CLASS_TAG]ParserTester::pr_item (const [STRUCT_TAG]& item) {
    fprintf(out(), "name: %s\n", item.name.c_str());
    [MEMBER_PRINT_TAG]
}

int [CLASS_TAG]ParserTester::run () {
    s_inst = this;
    NameToIdxCbs cbs = {};

    [DEP_CBS_WIRE_TAG]

    PathMng paths("../");

    [DEP_ITEMS_DECL_TAG]

    StringManager effect_items;

    [DEP_LD_TAG]

    const bool fx_ok = ld_sm(effect_items, paths.get_path_to_effects().c_str());
    if (fx_ok) {
        DataParserBase::set_item_effect_handler(&cbs, &effect_items);
    }

    [DEP_PARSER_DECL_TAG]

    [DEP_PSR_ASSIGN_TAG]

    StringManager raw_items;
    if (!raw_items.load_file_content("../game_config.[FILE_TAG]s")) {
        s_inst = NULL;
        return 0;
    }
    raw_items.split_string_by_char(0, '\n');
    raw_items.cull_empty_strings();
    if (raw_items.get_string_count() == 0) {
        s_inst = NULL;
        return 0;
    }
    [CLASS_TAG]Parser parser(raw_items, cbs);
    [STRUCT_TAG]* parsed_data = parser.parse_data_dependencies();
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
