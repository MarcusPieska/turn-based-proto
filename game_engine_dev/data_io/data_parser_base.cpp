//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>

#include "data_parser_base.h"
#include "item_effect_handler.h"
#include "str_mng.h"

//================================================================================================================================
//=> - DataParserBase implementation -
//================================================================================================================================

u32 DataParserBase::m_error_count = 0;
ItemEffectHandler* DataParserBase::s_item_effect_handler = nullptr;
const std::vector<RawItem>* DataParserBase::s_effect_definition_items = nullptr;

void DataParserBase::set_item_effect_handler (const NameToIdxCbs* name_to_idx_cbs, const std::vector<RawItem>* effect_defs) {
    if (s_item_effect_handler != nullptr) {
        return;
    }
    if (effect_defs == nullptr || effect_defs->empty() || name_to_idx_cbs == nullptr) {
        printf("ERROR: DataParserBase set_item_effect_handler: invalid parameters\n");
        ++m_error_count;
        return;
    }
    s_item_effect_handler = new ItemEffectHandler(name_to_idx_cbs);
    s_effect_definition_items = effect_defs;
}

void DataParserBase::clear_item_effect_handler () {
    if (s_item_effect_handler != nullptr) {
        delete s_item_effect_handler;
        s_item_effect_handler = nullptr;
    }
    if (s_effect_definition_items != nullptr) {
        delete s_effect_definition_items;
        s_effect_definition_items = nullptr;
    }
}

DataParserBase::DataParserBase (const std::vector<RawItem>& raw_items, const NameToIdxCbs& name_to_idx_cbs) :
    m_raw_items(raw_items),
    m_name_to_idx_cbs(name_to_idx_cbs) {
    if (raw_items.empty()) {
        printf("ERROR: DataParserBase received empty raw items\n");
        ++m_error_count;
        m_item_count = 0;
        return;
    }
    u16 upper_limit = -1; // This gets us the max value of u16 (65535)
    if (raw_items.size() > upper_limit) {
        printf("ERROR: DataParserBase received too many raw items\n");
        ++m_error_count;
        m_item_count = upper_limit;
        return;
    } else {
        m_item_count = static_cast<u16>(raw_items.size());
    }
}

u16 DataParserBase::name_to_idx (const std::string& name) const {
    for (size_t i = 0; i < m_item_count; ++i) {
        if (m_raw_items[i].name == name) {
            return static_cast<u16>(i);
        }
    }
    printf("ERROR: DataParserBase could not map name '%s' to index\n", name.c_str());
    ++m_error_count;
    return 0;
}

std::string DataParserBase::idx_to_name (u16 idx) const {
    if (idx >= m_item_count) {
        printf("ERROR: DataParserBase could not map index '%u' to name\n", idx);
        ++m_error_count;
        return "";
    }
    return m_raw_items[idx].name;
}

void DataParserBase::check_errors () {
    if (m_error_count > 0) {
        printf("ERROR: DataParserBase encountered %u parsing error(s)\n", static_cast<unsigned int>(m_error_count));
        std::exit(1);
    }
}

u32 DataParserBase::get_error_count_for_tests () {
    return m_error_count;
}

void DataParserBase::reset_error_count_for_tests () {
    m_error_count = 0;
}

const std::vector<RawItem>& DataParserBase::get_raw_items () const {
    return m_raw_items;
}

const std::vector<std::string> DataParserBase::get_line_items (const std::string& line) const {
    StringSplitter colon_splitter(":");
    StringTrimmer trimmer(" \t\r\n");

    std::vector<std::string> raw_parts = colon_splitter.split(line);
    std::vector<std::string> trimmed_parts;
    trimmed_parts.reserve(raw_parts.size());

    for (size_t i = 0; i < raw_parts.size(); ++i) {
        trimmed_parts.push_back(trimmer.trim(raw_parts[i]));
    }

    return trimmed_parts;
}

u16 DataParserBase::parse_u16 (const std::vector<std::string>& line_items, u16 start_idx) const {
    return std::stoi(line_items[start_idx]);
}

u32 DataParserBase::parse_u32 (const std::vector<std::string>& line_items, u16 start_idx) const {
    return std::stoi(line_items[start_idx]);
}

u16 DataParserBase::parse_unit_type (const std::vector<std::string>& line_items, u16 start_idx) const {
    return m_name_to_idx_cbs.unit_type_name_to_idx(line_items[start_idx].c_str());
}

ItemEffectsStruct DataParserBase::parse_item_effects (const std::vector<std::string>& line_items, u16 start_idx) const {
    (void)start_idx;
    ItemEffectsStruct effects = {};
    if (s_effect_definition_items == nullptr || s_effect_definition_items->empty()) {
        printf("ERROR: DataParserBase parse_item_effects: effect defs table not set or empty\n");
        ++m_error_count;
        return effects;
    }
    if (line_items.empty()) {
        printf("ERROR: DataParserBase parse_item_effects: empty line_items\n");
        ++m_error_count;
        return effects;
    }
    StringTrimmer trimmer(" \t\r\n");
    const std::string item_name = trimmer.trim(line_items[0]);
    if (item_name.empty()) {
        printf("ERROR: DataParserBase parse_item_effects: empty item name\n");
        ++m_error_count;
        return effects;
    }

    const RawItem* effect_row = nullptr;
    for (size_t i = 0; i < s_effect_definition_items->size(); ++i) {
        if (trimmer.trim((*s_effect_definition_items)[i].name) == item_name) {
            effect_row = &(*s_effect_definition_items)[i];
            break;
        }
    }
    if (effect_row == nullptr) {
        printf("ERROR: DataParserBase parse_item_effects: no effect row for item '%s'\n", item_name.c_str());
        ++m_error_count;
        return effects;
    }

    if (s_item_effect_handler == nullptr) {
        printf("ERROR: DataParserBase parse_item_effects: call DataParserBase::set_item_effect_handler first\n");
        ++m_error_count;
        return effects;
    }

    s_item_effect_handler->reset_error_count();
    effects = s_item_effect_handler->parse_effects_line(effect_row->raw_line);
    m_error_count += s_item_effect_handler->get_error_count();
    return effects;
}

ItemReqsStruct DataParserBase::parse_item_reqs (const std::vector<std::string>& line_items, u16 start_idx) const {
    ItemReqsStruct reqs;
    for (u16 i = 0; i < MAX_PREREQ_COUNT; ++i) {
        reqs.indices[i] = 0;
        reqs.types[i] = ITEM_REQ_TYPE_NONE;
        reqs.added_args[i] = 0;
    }
    StringTrimmer trimmer(" \t\r\n");
    StringExtractor arg_extractor("(", ")");
    StringSplitter type_splitter("(");
    StringSplitter comma_splitter(",");
    u16 write_idx = 0;
    for (u16 i = start_idx; i < line_items.size(); ++i) {
        std::string token = trimmer.trim(line_items[i]);
        if (token.empty()) {
            continue;
        }
        if (write_idx >= MAX_PREREQ_COUNT) {
            printf("ERROR: DataParserBase too many (%u) ItemReqsStruct items\n", MAX_PREREQ_COUNT);
            ++m_error_count;
            return reqs;
        }
        std::string req_name = token;
        if (arg_extractor.can_extract(token)) {
            std::string type_name = trimmer.trim(type_splitter.split(token)[0]);
            std::vector<std::string> args = comma_splitter.split(arg_extractor.extract(token));
            if (args.size() > 2) {
                printf("ERROR: DataParserBase too many (%lu) args in req token '%s'\n", args.size(), token.c_str());
                ++m_error_count;
                continue;
            }
            req_name = trimmer.trim(args[0]);
            if (req_name.empty()) {
                printf("ERROR: DataParserBase found empty req name in token '%s'\n", token.c_str());
                reqs.indices[write_idx] = 0;
                ++m_error_count;
                continue;
            }
            if (args.size() == 2) {
                reqs.added_args[write_idx] = static_cast<u8>(std::stoi(trimmer.trim(args[1])));
            }
            if (type_name == "resource") {
                reqs.types[write_idx] = ITEM_REQ_TYPE_RESOURCE;
                reqs.indices[write_idx] = m_name_to_idx_cbs.resource_name_to_idx(req_name.c_str());
            } else if (type_name == "flag") {
                reqs.types[write_idx] = ITEM_REQ_TYPE_FLAG;
                reqs.indices[write_idx] = m_name_to_idx_cbs.city_flag_name_to_idx(req_name.c_str());
            } else if (type_name == "civ") {
                reqs.types[write_idx] = ITEM_REQ_TYPE_CIV;
                reqs.indices[write_idx] = m_name_to_idx_cbs.civ_name_to_idx(req_name.c_str());
            } else if (type_name == "building") {
                reqs.types[write_idx] = ITEM_REQ_TYPE_BUILDING;
                reqs.indices[write_idx] = m_name_to_idx_cbs.building_name_to_idx(req_name.c_str());
            } else if (type_name == "tech") {
                reqs.types[write_idx] = ITEM_REQ_TYPE_TECH;
                reqs.indices[write_idx] = m_name_to_idx_cbs.tech_name_to_idx(req_name.c_str());
            } else {
                printf("ERROR: DataParserBase unknown req type '%s' in token '%s'\n", type_name.c_str(), token.c_str());
                ++m_error_count;
                continue;
            }
        } else {
            printf("ERROR: DataParserBase found invalid req token '%s'\n", token.c_str());
            ++m_error_count;
            continue;
        }
        ++write_idx;
    }
    return reqs;
}

CivTraitStruct DataParserBase::parse_civ_traits (const std::vector<std::string>& line_items, u16 start_idx) const {
    CivTraitStruct traits;
    for (u16 i = 0; i < MAX_CIV_TRAIT_COUNT; ++i) {
        traits.indices[i] = 0;
    }
    for (u16 i = 0; i + start_idx < line_items.size() && i < MAX_CIV_TRAIT_COUNT; ++i) {
        traits.indices[i] = m_name_to_idx_cbs.civ_trait_name_to_idx(line_items[i + start_idx].c_str());
    }
    return traits;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

