//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "building_parser.h"
#include "civ_bld_discount_map.h"
#include "civ_bld_discount_map_parsing.h"
#include "civ_trait_parser.h"
#include "name_to_idx_callbacks.h"
#include "opt_str_mng.h"
#include "static_bit_bank.h"

//================================================================================================================================
//=> - CivBldDiscountMapTester class -
//================================================================================================================================

class CivBldDiscountMapTester {
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
        const StringManager& civ_trait_items,
        const StringManager& building_items,
        u16 civ_trait_count,
        u16 building_count,
        const CivBldDiscountMap& map) {

        StringManager civ_name_parts;
        StringManager bld_name_parts;
        printf("CivBldDiscountMap (%u civ traits x %u buildings)\n", civ_trait_count, building_count);
        for (u16 c = 0; c < civ_trait_count; ++c) {
            cstr nm = get_name(civ_trait_items, c, civ_name_parts);
            if (std::strcmp(nm, "NONE") == 0) {
                continue;
            }
            printf("%s:\n", nm);
            for (u16 b = 0; b < building_count; ++b) {
                if (map.civ_trait_has_discount_for_bld(c, b)) {
                    cstr bnm = get_name(building_items, b, bld_name_parts);
                    if (std::strcmp(bnm, "NONE") == 0) {
                        continue;
                    }
                    printf("  - %s\n", bnm);
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
    StringManager building_items;
    StringManager civ_trait_items;
    building_items.load_file_content("../game_config.buildings");
    building_items.split_string_by_char(0, '\n');
    building_items.cull_empty_strings();
    civ_trait_items.load_file_content("../game_config.civ_traits");
    civ_trait_items.split_string_by_char(0, '\n');
    civ_trait_items.cull_empty_strings();
    BuildingParser building_parser(building_items, empty_cbs);
    CivTraitParser civ_trait_parser(civ_trait_items, empty_cbs);
    const u16 civ_trait_count = static_cast<u16>(civ_trait_items.get_string_count());
    const u16 building_count = static_cast<u16>(building_items.get_string_count());
    StaticBitBank* bank = new StaticBitBank(civ_trait_count, building_count);
    CivBldDiscountMapParsing::load_cfg_map(*bank, civ_trait_items, civ_trait_parser, building_parser);
    CivBldDiscountMap map;
    map.set_map(bank, civ_trait_count, building_count);
    map.take_ownership();
    CivBldDiscountMapTester::print_mapping(civ_trait_items, building_items, civ_trait_count, building_count, map);
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
