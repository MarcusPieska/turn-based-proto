//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_action_parser.h"

//================================================================================================================================
//=> - UnitActionParser implementation -
//================================================================================================================================

UnitActionParser::UnitActionParser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

UnitActionStaticDataStruct* UnitActionParser::parse_data_dependencies () {
    UnitActionStaticDataStruct* parsed_data = new UnitActionStaticDataStruct[m_item_count]();
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

