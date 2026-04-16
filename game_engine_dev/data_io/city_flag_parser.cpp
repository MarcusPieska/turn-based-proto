//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "city_flag_parser.h"

//================================================================================================================================
//=> - CityFlagParser implementation -
//================================================================================================================================

CityFlagParser::CityFlagParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

CityFlagStaticDataStruct* CityFlagParser::parse_data_dependencies () {
    CityFlagStaticDataStruct* parsed_data = new CityFlagStaticDataStruct[m_item_count]();
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

