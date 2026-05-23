//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "[FILE_TAG]_parser.h"

//================================================================================================================================
//=> - [CLASS_TAG]Parser implementation -
//================================================================================================================================

[CLASS_TAG]Parser::[CLASS_TAG]Parser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

[STRUCT_TAG]* [CLASS_TAG]Parser::parse_data_dependencies () {
    [STRUCT_TAG]* parsed_data = new [STRUCT_TAG][m_item_count]();
    for (u32 i = 0; i < m_item_count; ++i) {
        StringManager line_items;
        get_line_items(get_raw_lines().get_string_content(i), line_items);
        parsed_data[i].name = get_names().get_string_content(i);
        [PARSE_TAG]
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

