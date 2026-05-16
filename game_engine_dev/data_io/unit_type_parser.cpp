//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_type_parser.h"

//================================================================================================================================
//=> - UnitTypeParser implementation -
//================================================================================================================================

UnitTypeParser::UnitTypeParser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

UnitTypeStaticDataStruct* UnitTypeParser::parse_data_dependencies () {
    UnitTypeStaticDataStruct* parsed_data = new UnitTypeStaticDataStruct[m_item_count]();
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

