//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "city_flag_parser.h"

//================================================================================================================================
//=> - CityFlagParser implementation -
//================================================================================================================================

CityFlagParser::CityFlagParser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

CityFlagStaticDataStruct* CityFlagParser::parse_data_dependencies () {
    CityFlagStaticDataStruct* parsed_data = new CityFlagStaticDataStruct[m_item_count]();
    for (u32 i = 0; i < m_item_count; ++i) {
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

