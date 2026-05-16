//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "name_to_idx_callbacks.h"
#include "opt_str_mng.h"
#include "static_bit_bank.h"
#include "unit_action_parser.h"
#include "unit_type_action_map.h"
#include "unit_type_action_map_parsing.h"
#include "unit_type_parser.h"

//================================================================================================================================
//=> - UnitTypeActionMapTester class -
//================================================================================================================================

class UnitTypeActionMapTester {
public:
    static cstr get_name (const StringManager& items, u16 idx, StringManager& out_parts) {
        out_parts.load_cstr_content(items.get_string_content(idx));
        out_parts.split_string_by_char(0, ':');
        if (out_parts.get_string_count() == 0) {
            return "";
        }
        out_parts.trim_head_char(0, ' ');
        out_parts.trim_head_char(0, '\t');
        out_parts.trim_head_char(0, '\r');
        out_parts.trim_head_char(0, '\n');
        out_parts.trim_tail_char(0, ' ');
        out_parts.trim_tail_char(0, '\t');
        out_parts.trim_tail_char(0, '\r');
        out_parts.trim_tail_char(0, '\n');
        return out_parts.get_string_content(0);
    }

    static void print_mapping (
        const StringManager& unit_type_items,
        const StringManager& unit_action_items,
        u16 unit_type_count,
        u16 action_count) {

        StringManager unit_type_name_parts;
        StringManager action_name_parts;
        printf("UnitTypeActionMap (%u unit types x %u actions)\n", unit_type_count, action_count);
        for (u16 u = 0; u < unit_type_count; ++u) {
            cstr unm = get_name(unit_type_items, u, unit_type_name_parts);
            if (std::strcmp(unm, "NONE") == 0) {
                continue;
            }
            printf("%s:\n", unm);
            for (u16 a = 0; a < action_count; ++a) {
                if (UnitTypeActionMap::unit_type_can_do(u, a)) {
                    cstr anm = get_name(unit_action_items, a, action_name_parts);
                    if (std::strcmp(anm, "NONE") == 0) {
                        continue;
                    }
                    printf("  - %s\n", anm);
                }
            }
        }
    }
};

//================================================================================================================================
//=> - Main driver -
//================================================================================================================================

int main () {
    NameToIdxCbs empty_cbs{};
    StringManager unit_action_items;
    StringManager unit_type_items;
    unit_action_items.load_file_content("../game_config.unit_actions");
    unit_action_items.split_string_by_char(0, '\n');
    unit_action_items.cull_empty_strings();
    unit_type_items.load_file_content("../game_config.unit_types");
    unit_type_items.split_string_by_char(0, '\n');
    unit_type_items.cull_empty_strings();
    UnitTypeParser unit_type_parser(unit_type_items, empty_cbs);
    UnitActionParser unit_action_parser(unit_action_items, empty_cbs);
    const u16 unit_type_count = static_cast<u16>(unit_type_items.get_string_count());
    const u16 action_count = static_cast<u16>(unit_action_items.get_string_count());
    StaticBitBank bank(unit_type_count, action_count);
    UnitTypeActionMapParsing::load_cfg_map(bank, unit_type_items, unit_type_parser, unit_action_parser);
    UnitTypeActionMap::set_map(&bank, unit_type_count, action_count);
    UnitTypeActionMapTester::print_mapping(unit_type_items, unit_action_items, unit_type_count, action_count);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
