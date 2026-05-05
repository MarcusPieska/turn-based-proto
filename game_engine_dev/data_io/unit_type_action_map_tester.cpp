//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>

typedef std::string str;

#include "data_reader.h"
#include "name_to_idx_callbacks.h"
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
    static void print_mapping (
        const UnitTypeParser& unit_type_parser, 
        const UnitActionParser& unit_action_parser, 
        u16 unit_type_count, 
        u16 action_count) {
        
        printf("UnitTypeActionMap (%u unit types x %u actions)\n", unit_type_count, action_count);
        for (u16 u = 0; u < unit_type_count; ++u) {
            str unm = unit_type_parser.idx_to_name(u);
            if (unm == "NONE") {
                continue;
            }
            printf("%s:\n", unm.c_str());
            for (u16 a = 0; a < action_count; ++a) {
                if (UnitTypeActionMap::unit_type_can_do(u, a)) {
                    str anm = unit_action_parser.idx_to_name(a);
                    if (anm == "NONE") {
                        continue;
                    }
                    printf("  - %s\n", anm.c_str());
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
    DataReader unit_actions_reader("../game_config.unit_actions");
    DataReader unit_types_reader("../game_config.unit_types");
    UnitTypeParser unit_type_parser(unit_types_reader.get_raw_items(), empty_cbs);
    UnitActionParser unit_action_parser(unit_actions_reader.get_raw_items(), empty_cbs);
    const u16 unit_type_count = static_cast<u16>(unit_types_reader.get_raw_items().size());
    const u16 action_count = static_cast<u16>(unit_actions_reader.get_raw_items().size());
    StaticBitBank bank(unit_type_count, action_count);
    UnitTypeActionMapParsing::load_cfg_map(bank, unit_types_reader, unit_type_parser, unit_action_parser);
    UnitTypeActionMap::set_map(&bank, unit_type_count, action_count);
    UnitTypeActionMapTester::print_mapping(unit_type_parser, unit_action_parser, unit_type_count, action_count);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
