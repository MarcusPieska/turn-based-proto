//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_action_parser.h"

//================================================================================================================================
//=> - UnitActionParser implementation -
//================================================================================================================================

UnitActionParser::UnitActionParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

UnitActionStaticDataStruct* UnitActionParser::parse_data_dependencies () {
    UnitActionStaticDataStruct* parsed_data = new UnitActionStaticDataStruct[m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
        parsed_data[i].name = m_raw_items[i].name;
        // No parsing instructions provided
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

