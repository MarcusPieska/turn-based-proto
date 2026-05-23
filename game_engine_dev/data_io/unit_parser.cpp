//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include "unit_parser.h"

//================================================================================================================================
//=> - UnitParser implementation -
//================================================================================================================================

UnitParser::UnitParser (const StringManager& items, const NameToIdxCbs& map) : DataParserBase(items, map) {
}

UnitStaticDataStruct* UnitParser::parse_data_dependencies () {
    UnitStaticDataStruct* parsed_data = new UnitStaticDataStruct[m_item_count]();
    for (u32 i = 0; i < m_item_count; ++i) {
        StringManager line_items;
        get_line_items(get_raw_lines().get_string_content(i), line_items);
        parsed_data[i].name = get_names().get_string_content(i);
        parsed_data[i].type = parse_unit_type(line_items, 1);
        parsed_data[i].cost = parse_u32(line_items, 2);
        parsed_data[i].attack = parse_u16(line_items, 3);
        parsed_data[i].defense = parse_u16(line_items, 4);
        parsed_data[i].mvt_pts = parse_u16(line_items, 5);
        parsed_data[i].reqs = parse_item_reqs(line_items, 6);
    }
    return parsed_data;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

