//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_type_action_map_parsing.h"

//================================================================================================================================
//=> - UnitTypeActionMapParsing implementation -
//================================================================================================================================

void UnitTypeActionMapParsing::load_cfg_map (
    StaticBitBank& bank,
    const StringManager& mapping_items,
    const UnitTypeParser& unit_type_parser,
    const UnitActionParser& unit_action_parser
) {
    const u32 row_n = mapping_items.get_string_count();
    for (u32 row = 0; row < row_n; ++row) {
        StringManager parts;
        parts.load_cstr_content(mapping_items.get_string_content(row));
        parts.split_string_by_char(0, ':');
        const u32 part_n = parts.get_string_count();
        for (u32 i = 0; i < part_n; ++i) {
            parts.trim_head_char(i, ' ');
            parts.trim_head_char(i, '\t');
            parts.trim_head_char(i, '\r');
            parts.trim_head_char(i, '\n');
            parts.trim_tail_char(i, ' ');
            parts.trim_tail_char(i, '\t');
            parts.trim_tail_char(i, '\r');
            parts.trim_tail_char(i, '\n');
        }
        parts.cull_empty_strings();
        if (parts.get_string_count() < 2) {
            continue;
        }
        u16 type_idx = unit_type_parser.name_to_idx(parts.get_string_content(0));
        if (type_idx == U16_KEY_NULL) {
            continue;
        }
        const u32 act_n = parts.get_string_count();
        for (u32 i = 1; i < act_n; ++i) {
            u16 act_idx = unit_action_parser.name_to_idx(parts.get_string_content(i));
            if (act_idx == U16_KEY_NULL) {
                continue;
            }
            bank.set_flag(type_idx, act_idx);
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
