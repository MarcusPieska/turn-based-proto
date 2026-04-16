//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>
#include <vector>

#include "item_effect_handler.h"
#include "item_effect_helpers.h"
#include "name_to_idx_callbacks.h"
#include "str_mng.h"

//================================================================================================================================
//=> - Enable feature tokens (not item_effects enums) -
//================================================================================================================================

namespace {

bool enable_feature_from_token (const std::string& n, u16& out) {
    if (n == "ALL_GOVERNMENTS") {
        out = 1;
        return true;
    }
    if (n == "UNIT_VETERAN") {
        out = 2;
        return true;
    }
    if (n == "DIPLOMACY") {
        out = 3;
        return true;
    }
    if (n == "NUKES") {
        out = 4;
        return true;
    }
    if (n == "SPACE") {
        out = 5;
        return true;
    }
    if (n == "SHIP_BUILD") {
        out = 6;
        return true;
    }
    if (n == "AIR_UNIT") {
        out = 7;
        return true;
    }
    return false;
}

} // namespace

//================================================================================================================================
//=> - ItemEffectHandler implementation -
//================================================================================================================================

ItemEffectHandler::ItemEffectHandler (const NameToIdxCbs* name_to_idx_cbs) :
    m_cbs(name_to_idx_cbs),
    m_error_count(0) {
}

u32 ItemEffectHandler::get_error_count () const {
    return m_error_count;
}

void ItemEffectHandler::reset_error_count () {
    m_error_count = 0;
}

ItemEffectsStruct ItemEffectHandler::parse_effects_line (const std::string& line) const {
    ItemEffectsStruct effects = {};
    if (m_cbs == nullptr) {
        printf("ERROR: ItemEffectHandler null NameToIdxCbs\n");
        ++m_error_count;
        return effects;
    }

    StringSplitter colon_splitter(":");
    StringTrimmer trimmer(" \t\r\n");
    StringExtractor arg_extractor("(", ")");
    StringSplitter type_splitter("(");
    StringSplitter comma_splitter(",");

    std::vector<std::string> raw_parts = colon_splitter.split(line);
    std::vector<std::string> parts;
    parts.reserve(raw_parts.size());
    for (size_t i = 0; i < raw_parts.size(); ++i) {
        const std::string t = trimmer.trim(raw_parts[i]);
        if (!t.empty()) {
            parts.push_back(t);
        }
    }
    if (parts.size() < 2) {
        printf("ERROR: ItemEffectHandler line has no effect clauses\n");
        ++m_error_count;
        return effects;
    }

    u16 write_idx = 0;
    for (size_t pi = 1; pi < parts.size(); ++pi) {
        const std::string& token = parts[pi];
        if (write_idx >= MAX_EFFECT_COUNT) {
            printf("ERROR: ItemEffectHandler too many (%u) effects\n", MAX_EFFECT_COUNT);
            ++m_error_count;
            return effects;
        }
        if (!arg_extractor.can_extract(token)) {
            printf("ERROR: ItemEffectHandler invalid effect token '%s'\n", token.c_str());
            ++m_error_count;
            continue;
        }

        const std::string effect_verb = trimmer.trim(type_splitter.split(token)[0]);
        std::vector<std::string> args = comma_splitter.split(arg_extractor.extract(token));
        for (size_t a = 0; a < args.size(); ++a) {
            args[a] = trimmer.trim(args[a]);
        }

        if (m_cbs->effect_name_to_idx != nullptr) {
            (void)m_cbs->effect_name_to_idx(effect_verb.c_str());
        }

        ItemEffectStruct& slot = effects.items[write_idx];
        slot.type = static_cast<u16>(ItemEffectType::NONE);

        if (effect_verb == "booster") {
            if (args.size() != 4) {
                printf("ERROR: ItemEffectHandler booster(...) expects 4 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectBoosterType target_id = ItemEffectHelper::booster_type_str_to_enum(args[0]);
            if (target_id == ItemEffectBoosterType::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster target '%s'\n", args[0].c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args[2]);
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster scope '%s'\n", args[2].c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectAmountMode amount_mode = ItemEffectHelper::amount_mode_str_to_enum(args[3]);
            if (amount_mode == ItemEffectAmountMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster amount mode '%s'\n", args[3].c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::BOOSTER);
            slot.effect.booster.target_id = target_id;
            slot.effect.booster.amount = static_cast<i16>(std::stoi(args[1]));
            slot.effect.booster.scope = scope;
            slot.effect.booster.amount_mode = amount_mode;
        } else if (effect_verb == "build") {
            if (args.size() != 4) {
                printf("ERROR: ItemEffectHandler build(...) expects 4 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args[1]);
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown build scope '%s'\n", args[1].c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectBuildMode bmode = ItemEffectHelper::build_mode_str_to_enum(args[2]);
            if (bmode == ItemEffectBuildMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown build mode '%s'\n", args[2].c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectUpkeepMode umode = ItemEffectHelper::upkeep_mode_str_to_enum(args[3]);
            if (umode == ItemEffectUpkeepMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown upkeep mode '%s'\n", args[3].c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::BUILD);
            if (m_cbs->building_name_to_idx != nullptr) {
                slot.effect.build.building_id = m_cbs->building_name_to_idx(args[0].c_str());
            }
            slot.effect.build.scope = scope;
            slot.effect.build.build_mode = bmode;
            slot.effect.build.upkeep_mode = umode;
        } else if (effect_verb == "enable") {
            if (args.size() != 2) {
                printf("ERROR: ItemEffectHandler enable(...) expects 2 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            u16 feature_id = 0;
            if (!enable_feature_from_token(args[0], feature_id)) {
                printf("ERROR: ItemEffectHandler unknown enable feature '%s'\n", args[0].c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args[1]);
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown enable scope '%s'\n", args[1].c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::ENABLE);
            slot.effect.enable.feature_id = feature_id;
            slot.effect.enable.scope = scope;
        } else if (effect_verb == "researchTech") {
            if (args.size() != 1) {
                printf("ERROR: ItemEffectHandler researchTech(...) expects 1 arg in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::RESEARCH_TECH);
            slot.effect.research_tech.tech_count = static_cast<u16>(std::stoi(args[0]));
        } else if (effect_verb == "train") {
            if (args.size() != 2) {
                printf("ERROR: ItemEffectHandler train(...) expects 2 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::TRAIN);
            if (m_cbs->unit_name_to_idx != nullptr) {
                slot.effect.train.unit_id = m_cbs->unit_name_to_idx(args[0].c_str());
            }
            slot.effect.train.turns_interval = static_cast<u8>(std::stoi(args[1]));
        } else {
            printf("ERROR: ItemEffectHandler unhandled effect verb '%s'\n", effect_verb.c_str());
            ++m_error_count;
            continue;
        }
        ++write_idx;
    }
    return effects;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
