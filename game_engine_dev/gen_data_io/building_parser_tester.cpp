//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "building_parser.h"
#include "building_static_data.h"
#include "data_reader.h"
#include "path_mng.h"
#include "name_to_idx_callbacks.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;
int print_level = 0;

const DataParserBase* g_tech_parser = NULL;
const DataParserBase* g_resource_parser = NULL;
const DataParserBase* g_city_flag_parser = NULL;
const DataParserBase* g_building_parser = NULL;
const DataParserBase* g_civ_parser = NULL;

//================================================================================================================================
//=> - Helper functions -
//================================================================================================================================

u16 cb_tech_name_to_idx (cstr name) {
    return g_tech_parser->name_to_idx(name);
}

u16 cb_resource_name_to_idx (cstr name) {
    return g_resource_parser->name_to_idx(name);
}

u16 cb_city_flag_name_to_idx (cstr name) {
    return g_city_flag_parser->name_to_idx(name);
}

u16 cb_building_name_to_idx (cstr name) {
    return g_building_parser->name_to_idx(name);
}

u16 cb_civ_name_to_idx (cstr name) {
    return g_civ_parser->name_to_idx(name);
}

std::string req_idx_to_name (u8 req_type, u16 idx) {
    if (req_type == ITEM_REQ_TYPE_TECH && g_tech_parser != NULL) {
        return g_tech_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_RESOURCE && g_resource_parser != NULL) {
        return g_resource_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_FLAG && g_city_flag_parser != NULL) {
        return g_city_flag_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_CIV && g_civ_parser != NULL) {
        return g_civ_parser->idx_to_name(idx);
    }
    if (req_type == ITEM_REQ_TYPE_BUILDING && g_building_parser != NULL) {
        return g_building_parser->idx_to_name(idx);
    }
    return "<unknown>";
}

void print_item (const BuildingStaticDataStruct& item) {
    printf("name: %s\n", item.name.c_str());
    printf("  cost: %u\n", static_cast<unsigned int>(item.cost));
    printf("  reqs:\n");
    for (u32 j = 0; j < MAX_PREREQ_COUNT; ++j) {
        if (item.reqs.types[j] == ITEM_REQ_TYPE_NONE) {
            continue;
        }
        const u16 idx = item.reqs.indices[j];
        const u8 type = item.reqs.types[j];
        const std::string req_name = req_idx_to_name(type, idx);
        printf("    [%u] type=%u %s (%u)", j, type, req_name.c_str(), idx);
        if (item.reqs.added_args[j] != 0) {
            printf(" arg=%u", item.reqs.added_args[j]);
        }
        printf("\n");
    }
}

int run_parse_driver () {
    NameToIdxCbs cbs = {};
    cbs.tech_name_to_idx = cb_tech_name_to_idx;
    cbs.resource_name_to_idx = cb_resource_name_to_idx;
    cbs.city_flag_name_to_idx = cb_city_flag_name_to_idx;
    cbs.building_name_to_idx = cb_building_name_to_idx;
    cbs.civ_name_to_idx = cb_civ_name_to_idx;

    PathMng paths("../");
    DataReader tech_reader(paths.get_path_to_techs());
    DataReader resource_reader(paths.get_path_to_resources());
    DataReader city_flag_reader(paths.get_path_to_city_flags());
    DataReader building_reader(paths.get_path_to_buildings());
    DataReader civ_reader(paths.get_path_to_civs());

    DataParserBase tech_parser(tech_reader.get_raw_items(), cbs);
    DataParserBase resource_parser(resource_reader.get_raw_items(), cbs);
    DataParserBase city_flag_parser(city_flag_reader.get_raw_items(), cbs);
    DataParserBase building_parser(building_reader.get_raw_items(), cbs);
    DataParserBase civ_parser(civ_reader.get_raw_items(), cbs);

    g_tech_parser = &tech_parser;
    g_resource_parser = &resource_parser;
    g_city_flag_parser = &city_flag_parser;
    g_building_parser = &building_parser;
    g_civ_parser = &civ_parser;

    DataReader reader("../game_config.buildings");
    const std::vector<RawItem>& raw_items = reader.get_raw_items();

    BuildingParser parser(raw_items, cbs);
    BuildingStaticDataStruct* parsed_data = parser.parse_data_dependencies();

    if (print_level >= 2) {
        for (u32 i = 0; i < raw_items.size(); ++i) {
            print_item(parsed_data[i]);
            printf("--------------------------------\n");
        }
    }

    delete[] parsed_data;
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    return run_parse_driver();
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
