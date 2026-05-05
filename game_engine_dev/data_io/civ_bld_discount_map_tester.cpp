//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <string>

typedef std::string str;

#include "building_parser.h"
#include "civ_bld_discount_map.h"
#include "civ_bld_discount_map_parsing.h"
#include "civ_trait_parser.h"
#include "data_reader.h"
#include "name_to_idx_callbacks.h"
#include "static_bit_bank.h"

//================================================================================================================================
//=> - CivBldDiscountMapTester class -
//================================================================================================================================

class CivBldDiscountMapTester {
public:
    static void print_mapping (
        const CivTraitParser& civ_trait_parser, 
        const BuildingParser& building_parser, 
        u16 civ_trait_count,
        u16 building_count) {

        printf("CivBldDiscountMap (%u civ traits x %u buildings)\n", civ_trait_count, building_count);
        for (u16 c = 0; c < civ_trait_count; ++c) {
            str nm = civ_trait_parser.idx_to_name(c);
            if (nm == "NONE") {
                continue;
            }
            printf("%s:\n", nm.c_str());
            for (u16 b = 0; b < building_count; ++b) {
                if (CivBldDiscountMap::civ_trait_has_discount_for_bld(c, b)) {
                    str bnm = building_parser.idx_to_name(b);
                    if (bnm == "NONE") {
                        continue;
                    }
                    printf("  - %s\n", bnm.c_str());
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
    DataReader buildings_reader("../game_config.buildings");
    DataReader civ_traits_reader("../game_config.civ_traits");
    BuildingParser building_parser(buildings_reader.get_raw_items(), empty_cbs);
    CivTraitParser civ_trait_parser(civ_traits_reader.get_raw_items(), empty_cbs);
    const u16 civ_trait_count = static_cast<u16>(civ_traits_reader.get_raw_items().size());
    const u16 building_count = static_cast<u16>(buildings_reader.get_raw_items().size());
    StaticBitBank bank(civ_trait_count, building_count);
    CivBldDiscountMapParsing::load_cfg_map(bank, civ_traits_reader, civ_trait_parser, building_parser);
    CivBldDiscountMap::set_map(&bank, civ_trait_count, building_count);
    CivBldDiscountMapTester::print_mapping(civ_trait_parser, building_parser, civ_trait_count, building_count);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
