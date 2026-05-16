//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "civ_trait_parser.h"

//================================================================================================================================
//=> - CivTraitParser implementation -
//================================================================================================================================

CivTraitParser::CivTraitParser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

CivTraitStaticDataStruct* CivTraitParser::parse_data_dependencies () {
    CivTraitStaticDataStruct* parsed_data = new CivTraitStaticDataStruct[m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        StringManager line_items;
        get_line_items(get_raw_lines().get_string_content(i), line_items);
        parsed_data[i].name = get_names().get_string_content(i);
        // No parsing instructions provided
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

