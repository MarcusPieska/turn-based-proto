//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "resource_parser.h"

//================================================================================================================================
//=> - ResourceParser implementation -
//================================================================================================================================

ResourceParser::ResourceParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

ResourceStaticDataStruct* ResourceParser::parse_data_dependencies () {
    ResourceStaticDataStruct* parsed_data = new ResourceStaticDataStruct[m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
        parsed_data[i].name = m_raw_items[i].name;
        parsed_data[i].food = parse_u16(line_items, 1);
        parsed_data[i].shields = parse_u16(line_items, 2);
        parsed_data[i].commerce = parse_u16(line_items, 3);
        parsed_data[i].reqs = parse_item_reqs(line_items, 4);
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

