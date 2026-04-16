//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "civ_trait_parser.h"

//================================================================================================================================
//=> - CivTraitParser implementation -
//================================================================================================================================

CivTraitParser::CivTraitParser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

CivTraitStaticDataStruct* CivTraitParser::parse_data_dependencies () {
    CivTraitStaticDataStruct* parsed_data = new CivTraitStaticDataStruct[m_item_count]();
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

