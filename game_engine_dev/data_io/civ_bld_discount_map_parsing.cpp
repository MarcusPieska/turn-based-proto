//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <vector>

#include "civ_bld_discount_map_parsing.h"
#include "str_mng.h"

typedef std::string str;

//================================================================================================================================
//=> - CivBldDiscountMapParsing implementation -
//================================================================================================================================

void CivBldDiscountMapParsing::load_cfg_map (
    StaticBitBank& bank,
    const DataReader& mapping_reader,
    const CivTraitParser& civ_trait_parser,
    const BuildingParser& building_parser
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
        u16 civ_trait_idx = civ_trait_parser.name_to_idx(parts[0]);
        if (civ_trait_idx == 0) {
            continue;
        }
        for (size_t i = 1; i < parts.size(); ++i) {
            if (parts[i].empty()) {
                continue;
            }
            u16 building_idx = building_parser.name_to_idx(parts[i]);
            if (building_idx == 0) {
                continue;
            }
            bank.set_flag(civ_trait_idx, building_idx);
        }
    }
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
