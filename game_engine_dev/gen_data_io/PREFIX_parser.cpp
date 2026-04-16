//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "[FILE_TAG]_parser.h"

//================================================================================================================================
//=> - [CLASS_TAG]Parser implementation -
//================================================================================================================================

[CLASS_TAG]Parser::[CLASS_TAG]Parser (const std::vector<RawItem>& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

[STRUCT_TAG]* [CLASS_TAG]Parser::parse_data_dependencies () {
    [STRUCT_TAG]* parsed_data = new [STRUCT_TAG][m_item_count]();
    parsed_data[0].name = "NONE";
    for (u32 i = 1; i < m_item_count; ++i) {
        const std::vector<std::string> line_items = get_line_items(m_raw_items[i].raw_line);
        parsed_data[i].name = m_raw_items[i].name;
        [PARSE_TAG]
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

