//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "civ_parser.h"

//================================================================================================================================
//=> - CivParser implementation -
//================================================================================================================================

CivParser::CivParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

CivStaticDataStruct* CivParser::parse_data_dependencies () {
    CivStaticDataStruct* parsed_data = new CivStaticDataStruct[m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
        parsed_data[i].name = m_raw_items[i].name;
        parsed_data[i].traits = parse_civ_traits(line_items, 1);
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

