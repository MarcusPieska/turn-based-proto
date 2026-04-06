//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tech_data.h"
#include "building_data.h"
#include "resource_data.h"
#include "city_flags.h"
#include "wonder_data.h"

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_wonder_data () {
    u16 wonder_count = WonderData::get_wonder_data_count();
    const WonderTypeStats* wonders = WonderData::get_wonder_data_array();
    
    u16 tech_count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();
    
    u16 building_count = BuildingData::get_building_data_count();
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();
    
    u16 resource_count = ResourceData::get_resource_data_count();
    const ResourceTypeStats* resources = ResourceData::get_resource_data_array();
    
    u16 flag_count = CityFlagData::get_flag_count();
    const CityFlagStats* flags = CityFlagData::get_flag_data_array();

    printf("\n=======================================================\n");
    printf("WONDERS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < wonder_count; ++i) {
        const WonderTypeStats& wonder = wonders[i];
        
        printf("%s\n", wonder.name.c_str());
        printf("  Cost: %u\n", wonder.cost);
        
        u16 idx = wonder.tech_prereq_idx.get_idx();
        if (idx < tech_count) {
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[idx].name.c_str(), idx);
        } else {
            printf("  Tech Prerequisite: <invalid index %u>\n", idx);
        }
        
        bool has_reqs = false;
        for (u32 r = 0; r < MAX_WONDER_REQS; ++r) {
            const WonderRequirement& req = wonder.requirements[r];
            if (req.type == WONDER_REQ_NONE) {
                continue;
            }
            
            if (!has_reqs) {
                printf("  Requirements:\n");
                has_reqs = true;
            }
            
            switch (req.type) {
                case WONDER_REQ_FLAG: {
                    u16 idx = req.data.flag_req.flag_idx;
                    if (idx < flag_count) {
                        printf("    - Flag (%s)\n", flags[idx].name.c_str());
                    } else {
                        printf("    - Flag (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case WONDER_REQ_RESOURCE: {
                    u16 idx = req.data.resource_req.resource_idx;
                    if (idx < resource_count) {
                        printf("    - Resource (%s)\n", resources[idx].name.c_str());
                    } else {
                        printf("    - Resource (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case WONDER_REQ_BUILDING: {
                    u16 idx = req.data.building_req.building_idx;
                    u16 ct = req.data.building_req.count_required;
                    if (idx < building_count) {
                        printf("    - Building (%s, count=%u)\n", buildings[idx].name.c_str(), ct);
                    } else {
                        printf("    - Building (<invalid index %u>, %u)\n", idx, ct);
                    }
                    break;
                }
                default:
                    printf("    - <unknown requirement type %u>\n", req.type);
                    break;
            }
        }
        
        if (!has_reqs) {
            printf("  Requirements: None\n");
        }
        
        printf("\n");
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    TechData::load_static_data("../game_config.techs");
    CityFlagData::load_static_data("../game_config.city_flags");
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");
    WonderData::load_static_data("../game_config.wonders");

    print_wonder_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
