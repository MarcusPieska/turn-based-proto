//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <vector>

#include "unit_type_action_map_parsing.h"
#include "str_mng.h"

typedef std::string str;

//================================================================================================================================
//=> - UnitTypeActionMapParsing implementation -
//================================================================================================================================

void UnitTypeActionMapParsing::load_cfg_map (
    StaticBitBank& bank,
    const DataReader& mapping_reader,
    const UnitTypeParser& unit_type_parser,
    const UnitActionParser& unit_action_parser
) {
    const std::vector<RawItem>& items = mapping_reader.get_raw_items();
    StringTrimmer trimmer(" \t\r\n");
    StringSplitter splitter(":");
    for (size_t row = 1; row < items.size(); ++row) {
        str t = trimmer.trim(items[row].raw_line);
        if (t.empty()) {
            continue;
        }
        std::vector<str> parts = splitter.split(t);
        for (size_t i = 0; i < parts.size(); ++i) {
            parts[i] = trimmer.trim(parts[i]);
        }
        if (parts.size() < 2) {
            continue;
        }
        u16 type_idx = unit_type_parser.name_to_idx(parts[0]);
        if (type_idx == 0) {
            continue;
        }
        for (size_t i = 1; i < parts.size(); ++i) {
            if (parts[i].empty()) {
                continue;
            }
            u16 act_idx = unit_action_parser.name_to_idx(parts[i]);
            if (act_idx == 0) {
                continue;
            }
            bank.set_flag(type_idx, act_idx);
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
