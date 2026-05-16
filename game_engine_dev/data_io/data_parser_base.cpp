//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include "data_parser_base.h"
#include "item_effect_handler.h"

//================================================================================================================================
//=> - DataParserBase implementation -
//================================================================================================================================

u32 DataParserBase::m_error_count = 0;
ItemEffectHandler* DataParserBase::s_item_effect_handler = 0;
const StringManager* DataParserBase::s_effect_definition_items = 0;

static std::string trim_ws(cstr in) {
    if (!in) return "";
    std::string s(in);
    std::size_t b = 0;
    std::size_t e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' || s[b] == '\n')) ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' || s[e - 1] == '\n')) --e;
    return s.substr(b, e - b);
}

void DataParserBase::set_item_effect_handler (const NameToIdxCbs* name_to_idx_cbs, const StringManager* effect_defs) {
    if (s_item_effect_handler != 0) {
        return;
    }
    if (effect_defs == 0 || effect_defs->get_string_count() == 0 || name_to_idx_cbs == 0) {
        printf("ERROR: DataParserBase set_item_effect_handler: invalid parameters\n");
        ++m_error_count;
        return;
    }
    s_item_effect_handler = new ItemEffectHandler(name_to_idx_cbs);
    s_effect_definition_items = effect_defs;
}

void DataParserBase::clear_item_effect_handler () {
    if (s_item_effect_handler != 0) {
        delete s_item_effect_handler;
        s_item_effect_handler = 0;
    }
    s_effect_definition_items = 0;
}

void DataParserBase::derive_names_from_raw_lines (const StringManager& raw_lines, StringManager& out_names) const {
    u32 n = raw_lines.get_string_count();
    u32 total = 0;
    for (u32 i = 0; i < n; ++i) {
        StringManager parts;
        parts.load_cstr_content(raw_lines.get_string_content(i));
        parts.split_string_by_char(0, ':');
        if (parts.get_string_count() == 0) continue;
        parts.trim_head_char(0, ' ');
        parts.trim_tail_char(0, ' ');
        parts.trim_head_char(0, '\t');
        parts.trim_tail_char(0, '\t');
        parts.trim_head_char(0, '\r');
        parts.trim_tail_char(0, '\r');
        total += (u32)std::strlen(parts.get_string_content(0)) + 1;
    }
    if (total == 0) {
        out_names.load_cstr_content("");
        out_names.cull_empty_strings();
        return;
    }
    char* buf = new char[(std::size_t)total];
    u32 k = 0;
    for (u32 i = 0; i < n; ++i) {
        StringManager parts;
        parts.load_cstr_content(raw_lines.get_string_content(i));
        parts.split_string_by_char(0, ':');
        if (parts.get_string_count() == 0) continue;
        parts.trim_head_char(0, ' ');
        parts.trim_tail_char(0, ' ');
        parts.trim_head_char(0, '\t');
        parts.trim_tail_char(0, '\t');
        parts.trim_head_char(0, '\r');
        parts.trim_tail_char(0, '\r');
        cstr name = parts.get_string_content(0);
        std::size_t len = std::strlen(name);
        std::memcpy(buf + k, name, len);
        k += (u32)len;
        buf[k++] = '\n';
    }
    if (k > 0) --k;
    buf[k] = '\0';
    out_names.load_cstr_content(buf);
    out_names.split_string_by_char(0, '\n');
    out_names.cull_empty_strings();
    delete[] buf;
}

DataParserBase::DataParserBase (const StringManager& raw_lines, const NameToIdxCbs& name_to_idx_cbs) :
    m_name_to_idx_cbs(name_to_idx_cbs),
    m_item_count(0) {
    if (raw_lines.get_string_count() == 0) {
        printf("ERROR: DataParserBase received empty raw items\n");
        ++m_error_count;
        return;
    }
    u32 total = 0;
    for (u32 i = 0; i < raw_lines.get_string_count(); ++i) {
        total += (u32)std::strlen(raw_lines.get_string_content(i)) + 1;
    }
    char* buf = new char[(std::size_t)total];
    u32 k = 0;
    for (u32 i = 0; i < raw_lines.get_string_count(); ++i) {
        cstr line = raw_lines.get_string_content(i);
        std::size_t len = std::strlen(line);
        std::memcpy(buf + k, line, len);
        k += (u32)len;
        buf[k++] = '\n';
    }
    if (k > 0) --k;
    buf[k] = '\0';
    m_raw_lines.load_cstr_content(buf);
    m_raw_lines.split_string_by_char(0, '\n');
    m_raw_lines.cull_empty_strings();
    delete[] buf;

    derive_names_from_raw_lines(m_raw_lines, m_names);

    u16 upper_limit = (u16)-1;
    if (m_raw_lines.get_string_count() > upper_limit) {
        printf("ERROR: DataParserBase received too many raw items\n");
        ++m_error_count;
        m_item_count = upper_limit;
        return;
    }
    m_item_count = (u16)m_raw_lines.get_string_count();
}

u16 DataParserBase::name_to_idx (cstr name) const {
    std::string target = trim_ws(name);
    for (u32 i = 0; i < m_item_count; ++i) {
        if (trim_ws(m_names.get_string_content(i)) == target) {
            return (u16)i;
        }
    }
    printf("ERROR: DataParserBase could not map name '%s' to index\n", name ? name : "");
    ++m_error_count;
    return U16_KEY_NULL;
}

std::string DataParserBase::idx_to_name (u16 idx) const {
    if (idx >= m_item_count) {
        printf("ERROR: DataParserBase could not map index '%u' to name\n", idx);
        ++m_error_count;
        return "";
    }
    return m_names.get_string_content(idx);
}

void DataParserBase::check_errors () {
    if (m_error_count > 0) {
        printf("ERROR: DataParserBase encountered %u parsing error(s)\n", (unsigned int)m_error_count);
        std::exit(1);
    }
}

u32 DataParserBase::get_error_count_for_tests () {
    return m_error_count;
}

void DataParserBase::reset_error_count_for_tests () {
    m_error_count = 0;
}

const StringManager& DataParserBase::get_raw_lines () const {
    return m_raw_lines;
}

const StringManager& DataParserBase::get_names () const {
    return m_names;
}

void DataParserBase::get_line_items (cstr line, StringManager& out_items) const {
    out_items.load_cstr_content(line ? line : "");
    out_items.split_string_by_char(0, ':');
    for (u32 i = 0; i < out_items.get_string_count(); ++i) {
        out_items.trim_head_char(i, ' ');
        out_items.trim_tail_char(i, ' ');
        out_items.trim_head_char(i, '\t');
        out_items.trim_tail_char(i, '\t');
        out_items.trim_head_char(i, '\r');
        out_items.trim_tail_char(i, '\r');
        out_items.trim_head_char(i, '\n');
        out_items.trim_tail_char(i, '\n');
    }
}

u16 DataParserBase::parse_u16 (const StringManager& line_items, u16 start_idx) const {
    return (u16)std::strtoul(line_items.get_string_content(start_idx), 0, 10);
}

u32 DataParserBase::parse_u32 (const StringManager& line_items, u16 start_idx) const {
    return (u32)std::strtoul(line_items.get_string_content(start_idx), 0, 10);
}

u16 DataParserBase::parse_unit_type (const StringManager& line_items, u16 start_idx) const {
    return m_name_to_idx_cbs.unit_type_name_to_idx(line_items.get_string_content(start_idx));
}

ItemEffectsStruct DataParserBase::parse_item_effects (const StringManager& line_items, u16 start_idx) const {
    (void)start_idx;
    ItemEffectsStruct effects = {};
    if (s_effect_definition_items == 0 || s_effect_definition_items->get_string_count() == 0) {
        printf("ERROR: DataParserBase parse_item_effects: effect defs table not set or empty\n");
        ++m_error_count;
        return effects;
    }
    if (line_items.get_string_count() == 0) {
        printf("ERROR: DataParserBase parse_item_effects: empty line_items\n");
        ++m_error_count;
        return effects;
    }
    std::string item_name = trim_ws(line_items.get_string_content(0));
    if (item_name.empty()) {
        printf("ERROR: DataParserBase parse_item_effects: empty item name\n");
        ++m_error_count;
        return effects;
    }

    cstr effect_line = 0;
    for (u32 i = 0; i < s_effect_definition_items->get_string_count(); ++i) {
        StringManager parts;
        get_line_items(s_effect_definition_items->get_string_content(i), parts);
        if (trim_ws(parts.get_string_content(0)) == item_name) {
            effect_line = s_effect_definition_items->get_string_content(i);
            break;
        }
    }
    if (effect_line == 0) {
        printf("ERROR: DataParserBase parse_item_effects: no effect row for item '%s'\n", item_name.c_str());
        ++m_error_count;
        return effects;
    }
    if (s_item_effect_handler == 0) {
        printf("ERROR: DataParserBase parse_item_effects: call DataParserBase::set_item_effect_handler first\n");
        ++m_error_count;
        return effects;
    }
    s_item_effect_handler->reset_error_count();
    effects = s_item_effect_handler->parse_effects_line(std::string(effect_line));
    m_error_count += s_item_effect_handler->get_error_count();
    return effects;
}

ItemReqsStruct DataParserBase::parse_item_reqs (const StringManager& line_items, u16 start_idx) const {
    ItemReqsStruct reqs = {};
    u16 write_idx = 0;
    for (u16 i = start_idx; i < line_items.get_string_count(); ++i) {
        std::string token = trim_ws(line_items.get_string_content(i));
        if (token.empty()) {
            continue;
        }
        if (write_idx >= MAX_PREREQ_COUNT) {
            printf("ERROR: DataParserBase too many (%u) ItemReqsStruct items\n", MAX_PREREQ_COUNT);
            ++m_error_count;
            return reqs;
        }
        std::size_t p0 = token.find('(');
        std::size_t p1 = token.rfind(')');
        if (p0 == std::string::npos || p1 == std::string::npos || p1 <= p0 + 1) {
            printf("ERROR: DataParserBase found invalid req token '%s'\n", token.c_str());
            ++m_error_count;
            continue;
        }
        std::string type_name = trim_ws(token.substr(0, p0).c_str());
        std::string inside = token.substr(p0 + 1, p1 - p0 - 1);
        std::size_t comma = inside.find(',');
        std::string req_name = trim_ws((comma == std::string::npos ? inside : inside.substr(0, comma)).c_str());
        if (req_name.empty()) {
            printf("ERROR: DataParserBase found empty req name in token '%s'\n", token.c_str());
            ++m_error_count;
            continue;
        }
        if (comma != std::string::npos) {
            reqs.added_args[write_idx] = (u8)std::strtoul(trim_ws(inside.substr(comma + 1).c_str()).c_str(), 0, 10);
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
        ++write_idx;
    }
    return reqs;
}

CivTraitStruct DataParserBase::parse_civ_traits (const StringManager& line_items, u16 start_idx) const {
    CivTraitStruct traits = {};
    for (u16 i = 0; i + start_idx < line_items.get_string_count() && i < MAX_CIV_TRAIT_COUNT; ++i) {
        traits.indices[i] = m_name_to_idx_cbs.civ_trait_name_to_idx(line_items.get_string_content(i + start_idx));
    }
    return traits;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

