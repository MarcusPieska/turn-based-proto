//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdlib>
#include <cstring>

#include "trait_affinity_map_parsing.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void trim_part (StringManager& parts, u32 i) {
    parts.trim_head_char(i, ' ');
    parts.trim_head_char(i, '\t');
    parts.trim_head_char(i, '\r');
    parts.trim_head_char(i, '\n');
    parts.trim_tail_char(i, ' ');
    parts.trim_tail_char(i, '\t');
    parts.trim_tail_char(i, '\r');
    parts.trim_tail_char(i, '\n');
}

static u16 parse_base (cstr s) {
    if (!s || s[0] == '\0') {
        return 0;
    }
    return static_cast<u16>(std::strtoul(s, nullptr, 10));
}

static bool parse_row (
    const StringManager& items,
    u32 line,
    const CivTraitParser& civ_trait_parser,
    TraitAffinityRowStruct* out_row,
    u32* out_char_n
) {
    StringManager parts;
    parts.load_cstr_content(items.get_string_content(line));
    parts.split_string_by_char(0, ':');
    const u32 part_n = parts.get_string_count();
    for (u32 i = 0; i < part_n; ++i) {
        trim_part(parts, i);
    }
    parts.cull_empty_strings();
    if (parts.get_string_count() < 3) {
        return false;
    }
    cstr token = parts.get_string_content(0);
    if (!token || token[0] == '\0') {
        return false;
    }
    TraitAffinityRowStruct row = {};
    row.m_base = parse_base(parts.get_string_content(1));
    const u32 trait_n = parts.get_string_count();
    for (u32 i = 2; i < trait_n && row.m_trait_n < TRAIT_AFFINITY_ROW_TRAIT_MAX; ++i) {
        u16 trait_idx = civ_trait_parser.name_to_idx(parts.get_string_content(i));
        if (trait_idx == U16_KEY_NULL) {
            continue;
        }
        row.m_traits[row.m_trait_n] = trait_idx;
        ++row.m_trait_n;
    }
    if (row.m_trait_n == 0) {
        return false;
    }
    *out_row = row;
    *out_char_n = (u32)std::strlen(token);
    return true;
}

static bool fill_row_and_token (
    const StringManager& items,
    u32 line,
    const CivTraitParser& civ_trait_parser,
    TraitAffinityRowStruct* out_row,
    StringManager& token_out
) {
    token_out.load_cstr_content(items.get_string_content(line));
    token_out.split_string_by_char(0, ':');
    const u32 part_n = token_out.get_string_count();
    for (u32 i = 0; i < part_n; ++i) {
        trim_part(token_out, i);
    }
    token_out.cull_empty_strings();
    if (token_out.get_string_count() < 3) {
        return false;
    }
    cstr token = token_out.get_string_content(0);
    if (!token || token[0] == '\0') {
        return false;
    }
    TraitAffinityRowStruct row = {};
    row.m_base = parse_base(token_out.get_string_content(1));
    const u32 trait_n = token_out.get_string_count();
    for (u32 i = 2; i < trait_n && row.m_trait_n < TRAIT_AFFINITY_ROW_TRAIT_MAX; ++i) {
        u16 trait_idx = civ_trait_parser.name_to_idx(token_out.get_string_content(i));
        if (trait_idx == U16_KEY_NULL) {
            continue;
        }
        row.m_traits[row.m_trait_n] = trait_idx;
        ++row.m_trait_n;
    }
    if (row.m_trait_n == 0) {
        return false;
    }
    *out_row = row;
    return true;
}

//================================================================================================================================
//=> - TraitAffinityMapParsing -
//================================================================================================================================

bool TraitAffinityMapParsing::load_cfg (
    TraitAffinityMap& out,
    const StringManager& items,
    const CivTraitParser& civ_trait_parser
) {
    const u32 line_n = items.get_string_count();
    if (line_n == 0) {
        return false;
    }
    u16 row_n = 0;
    u32 char_n = 0;
    for (u32 line = 0; line < line_n; ++line) {
        TraitAffinityRowStruct row = {};
        u32 token_n = 0;
        if (!parse_row(items, line, civ_trait_parser, &row, &token_n)) {
            continue;
        }
        char_n += token_n;
        ++row_n;
    }
    if (row_n == 0) {
        return false;
    }
    TraitAffinityRowStruct* rows = new TraitAffinityRowStruct[row_n];
    StaticStringPool pool(row_n, char_n);
    StringManager token_parts;
    u16 row_i = 0;
    for (u32 line = 0; line < line_n; ++line) {
        TraitAffinityRowStruct row = {};
        if (!fill_row_and_token(items, line, civ_trait_parser, &row, token_parts)) {
            continue;
        }
        rows[row_i] = row;
        u16 k = 0;
        if (!pool.add(token_parts.get_string_content(0), &k) || k != row_i) {
            delete[] rows;
            return false;
        }
        ++row_i;
    }
    out.set_rows(rows, row_n, pool);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
