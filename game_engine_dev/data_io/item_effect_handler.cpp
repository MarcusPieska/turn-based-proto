//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <string>

#include "item_effect_handler.h"
#include "item_effect_helpers.h"
#include "name_to_idx_callbacks.h"

//================================================================================================================================
//=> - Enable feature tokens (not item_effects enums) -
//================================================================================================================================

namespace {

std::string trim_copy (cstr in) {
    if (!in) return "";
    std::string s(in);
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

void trim_all (StringManager& m) {
    for (u32 i = 0; i < m.get_string_count(); ++i) {
        m.trim_head_char(i, ' ');
        m.trim_tail_char(i, ' ');
        m.trim_head_char(i, '\t');
        m.trim_tail_char(i, '\t');
        m.trim_head_char(i, '\r');
        m.trim_tail_char(i, '\r');
        m.trim_head_char(i, '\n');
        m.trim_tail_char(i, '\n');
    }
}

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

ItemEffectsStruct ItemEffectHandler::parse_effects_line (const StringManager& line_items) const {
    ItemEffectsStruct effects = {};
    if (m_cbs == nullptr) {
        printf("ERROR: ItemEffectHandler null NameToIdxCbs\n");
        ++m_error_count;
        return effects;
    }
    if (line_items.get_string_count() < 2) {
        printf("ERROR: ItemEffectHandler line has no effect clauses\n");
        ++m_error_count;
        return effects;
    }

    u16 write_idx = 0;
    for (u32 pi = 1; pi < line_items.get_string_count(); ++pi) {
        const std::string token = trim_copy(line_items.get_string_content(pi));
        if (write_idx >= MAX_EFFECT_COUNT) {
            printf("ERROR: ItemEffectHandler too many (%u) effects\n", MAX_EFFECT_COUNT);
            ++m_error_count;
            return effects;
        }
        std::size_t p0 = token.find('(');
        std::size_t p1 = token.rfind(')');
        if (p0 == std::string::npos || p1 == std::string::npos || p1 <= p0 + 1) {
            printf("ERROR: ItemEffectHandler invalid effect token '%s'\n", token.c_str());
            ++m_error_count;
            continue;
        }

        const std::string effect_verb = trim_copy(token.substr(0, p0).c_str());
        const std::string arg_text = token.substr(p0 + 1, p1 - p0 - 1);
        StringManager args_mgr;
        args_mgr.load_cstr_content(arg_text.c_str());
        args_mgr.split_string_by_char(0, ',');
        trim_all(args_mgr);
        args_mgr.cull_empty_strings();
        u32 arg_n = args_mgr.get_string_count();

        if (m_cbs->effect_name_to_idx != nullptr) {
            (void)m_cbs->effect_name_to_idx(effect_verb.c_str());
        }

        ItemEffectStruct& slot = effects.items[write_idx];
        slot.type = static_cast<u16>(ItemEffectType::NONE);

        if (effect_verb == "booster") {
            if (arg_n != 4) {
                printf("ERROR: ItemEffectHandler booster(...) expects 4 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectBoosterType target_id = ItemEffectHelper::booster_type_str_to_enum(args_mgr.get_string_content(0));
            if (target_id == ItemEffectBoosterType::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster target '%s'\n", args_mgr.get_string_content(0));
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args_mgr.get_string_content(2));
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster scope '%s'\n", args_mgr.get_string_content(2));
                ++m_error_count;
                continue;
            }
            const ItemEffectAmountMode amount_mode = ItemEffectHelper::amount_mode_str_to_enum(args_mgr.get_string_content(3));
            if (amount_mode == ItemEffectAmountMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown booster amount mode '%s'\n", args_mgr.get_string_content(3));
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::BOOSTER);
            slot.effect.booster.target_id = target_id;
            slot.effect.booster.amount = static_cast<i16>(std::stoi(args_mgr.get_string_content(1)));
            slot.effect.booster.scope = scope;
            slot.effect.booster.amount_mode = amount_mode;
        } else if (effect_verb == "build") {
            if (arg_n != 4) {
                printf("ERROR: ItemEffectHandler build(...) expects 4 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args_mgr.get_string_content(1));
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown build scope '%s'\n", args_mgr.get_string_content(1));
                ++m_error_count;
                continue;
            }
            const ItemEffectBuildMode bmode = ItemEffectHelper::build_mode_str_to_enum(args_mgr.get_string_content(2));
            if (bmode == ItemEffectBuildMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown build mode '%s'\n", args_mgr.get_string_content(2));
                ++m_error_count;
                continue;
            }
            const ItemEffectUpkeepMode umode = ItemEffectHelper::upkeep_mode_str_to_enum(args_mgr.get_string_content(3));
            if (umode == ItemEffectUpkeepMode::NONE) {
                printf("ERROR: ItemEffectHandler unknown upkeep mode '%s'\n", args_mgr.get_string_content(3));
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::BUILD);
            slot.effect.build.building_id = U16_KEY_NULL;
            if (m_cbs->building_name_to_idx != nullptr) {
                slot.effect.build.building_id = m_cbs->building_name_to_idx(args_mgr.get_string_content(0));
            }
            slot.effect.build.scope = scope;
            slot.effect.build.build_mode = bmode;
            slot.effect.build.upkeep_mode = umode;
        } else if (effect_verb == "enable") {
            if (arg_n != 2) {
                printf("ERROR: ItemEffectHandler enable(...) expects 2 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            u16 feature_id = 0;
            if (!enable_feature_from_token(args_mgr.get_string_content(0), feature_id)) {
                printf("ERROR: ItemEffectHandler unknown enable feature '%s'\n", args_mgr.get_string_content(0));
                ++m_error_count;
                continue;
            }
            const ItemEffectsScope scope = ItemEffectHelper::effects_scope_str_to_enum(args_mgr.get_string_content(1));
            if (scope == ItemEffectsScope::NONE) {
                printf("ERROR: ItemEffectHandler unknown enable scope '%s'\n", args_mgr.get_string_content(1));
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::ENABLE);
            slot.effect.enable.feature_id = feature_id;
            slot.effect.enable.scope = scope;
        } else if (effect_verb == "researchTech") {
            if (arg_n != 1) {
                printf("ERROR: ItemEffectHandler researchTech(...) expects 1 arg in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::RESEARCH_TECH);
            slot.effect.research_tech.tech_count = static_cast<u16>(std::stoi(args_mgr.get_string_content(0)));
        } else if (effect_verb == "train") {
            if (arg_n != 2) {
                printf("ERROR: ItemEffectHandler train(...) expects 2 args in '%s'\n", token.c_str());
                ++m_error_count;
                continue;
            }
            slot.type = static_cast<u16>(ItemEffectType::TRAIN);
            slot.effect.train.unit_id = U16_KEY_NULL;
            if (m_cbs->unit_name_to_idx != nullptr) {
                slot.effect.train.unit_id = m_cbs->unit_name_to_idx(args_mgr.get_string_content(0));
            }
            slot.effect.train.turns_interval = static_cast<u8>(std::stoi(args_mgr.get_string_content(1)));
        } else {
            printf("ERROR: ItemEffectHandler unhandled effect verb '%s'\n", effect_verb.c_str());
            ++m_error_count;
            continue;
        }
        ++write_idx;
    }
    return effects;
}

ItemEffectsStruct ItemEffectHandler::parse_effects_line (cstr line) const {
    StringManager parts;
    parts.load_cstr_content(line ? line : "");
    parts.split_string_by_char(0, ':');
    trim_all(parts);
    parts.cull_empty_strings();
    return parse_effects_line(parts);
}

ItemEffectsStruct ItemEffectHandler::parse_effects_line (const std::string& line) const {
    return parse_effects_line(line.c_str());
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
