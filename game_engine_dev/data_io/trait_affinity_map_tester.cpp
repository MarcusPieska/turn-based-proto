//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "civ_trait_parser.h"
#include "name_to_idx_callbacks.h"
#include "opt_str_mng.h"
#include "trait_affinity_map.h"
#include "trait_affinity_map_parsing.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static cstr trait_name (const StringManager& items, u16 idx, StringManager& parts) {
    parts.load_cstr_content(items.get_string_content(idx));
    parts.split_string_by_char(0, ':');
    if (parts.get_string_count() == 0) {
        return "";
    }
    parts.trim_head_char(0, ' ');
    parts.trim_head_char(0, '\t');
    parts.trim_tail_char(0, ' ');
    parts.trim_tail_char(0, '\t');
    return parts.get_string_content(0);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    NameToIdxCbs empty_cbs{};
    StringManager civ_trait_items;
    StringManager trait_affinity_items;
    civ_trait_items.load_file_content("../game_config.civ_traits");
    civ_trait_items.split_string_by_char(0, '\n');
    civ_trait_items.cull_empty_strings();
    trait_affinity_items.load_file_content("../game_config.trait_affinity");
    trait_affinity_items.split_string_by_char(0, '\n');
    trait_affinity_items.cull_empty_strings();
    CivTraitParser civ_trait_parser(civ_trait_items, empty_cbs);
    TraitAffinityMap map;
    if (!TraitAffinityMapParsing::load_cfg(map, trait_affinity_items, civ_trait_parser)) {
        printf("TraitAffinityMap load failed\n");
        return 1;
    }
    map.take_ownership();
    printf("TraitAffinityMap (%u rows)\n", map.get_row_count());
    StringManager trait_parts;
    for (u16 i = 0; i < map.get_row_count(); ++i) {
        const TraitAffinityRowStruct& row = map.get_row(i);
        printf("%s : %u", map.get_token_name(i), row.m_base);
        for (u8 t = 0; t < row.m_trait_n; ++t) {
            printf(" : %s", trait_name(civ_trait_items, row.m_traits[t], trait_parts));
        }
        printf("\n");
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
