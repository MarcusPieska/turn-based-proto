//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "tech_parser.h"

//================================================================================================================================
//=> - TechParser implementation -
//================================================================================================================================

TechParser::TechParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

TechStaticDataStruct* TechParser::parse_data_dependencies () {
    TechStaticDataStruct* parsed_data = new TechStaticDataStruct[m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
        parsed_data[i].name = m_raw_items[i].name;
        parsed_data[i].cost = parse_u32(line_items, 1);
        parsed_data[i].reqs = parse_item_reqs(line_items, 2);
        parsed_data[i].effects = parse_item_effects(line_items, 3);
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

