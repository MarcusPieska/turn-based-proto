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
#include "small_wonder_data.h"

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_wonder_small_data () {
    u16 wonder_count = SmallWonderData::get_small_wonder_data_count();
    const SmallWonderTypeStats* wonders = SmallWonderData::get_small_wonder_data_array();
    
    u16 tech_count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();
    
    u16 building_count = BuildingData::get_building_data_count();
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();
    
    u16 resource_count = ResourceData::get_resource_data_count();
    const ResourceTypeStats* resources = ResourceData::get_resource_data_array();
    
    u16 flag_count = CityFlagData::get_flag_count();
    const CityFlagStats* flags = CityFlagData::get_flag_data_array();

    printf("\n=======================================================\n");
    printf("SMALL WONDERS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < wonder_count; ++i) {
        const SmallWonderTypeStats& wonder = wonders[i];
        
        printf("%s\n", wonder.name.c_str());
        printf("  Cost: %u\n", static_cast<u32>(wonder.cost));
        
        u16 tech_idx = wonder.tech_prereq_idx.get_idx();
        if (tech_idx < tech_count) {
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[tech_idx].name.c_str(), tech_idx);
        } else {
            printf("  Tech Prerequisite: <invalid index %u>\n", tech_idx);
        }
        
        bool has_reqs = false;
        for (u32 r = 0; r < MAX_WONDER_SMALL_REQS; ++r) {
            const SmallWonderRequirement& req = wonder.requirements[r];
            if (req.type == SMALL_WONDER_REQ_NONE) {
                continue;
            }
            if (!has_reqs) {
                printf("  Requirements:\n");
                has_reqs = true;
            }
            
            switch (req.type) {
                case SMALL_WONDER_REQ_FLAG: {
                    u32 idx = static_cast<u32>(req.data.flag_req.flag_idx);
                    if (idx < flag_count) {
                        printf("    - Flag (%s)\n", flags[idx].name.c_str());
                    } else {
                        printf("    - Flag (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case SMALL_WONDER_REQ_RESOURCE: {
                    u32 idx = static_cast<u32>(req.data.resource_req.resource_idx);
                    if (idx < resource_count) {
                        printf("    - Resource (%s)\n", resources[idx].name.c_str());
                    } else {
                        printf("    - Resource (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case SMALL_WONDER_REQ_BUILDING: {
                    u32 idx = static_cast<u32>(req.data.building_req.building_idx);
                    u32 ct = static_cast<u32>(req.data.building_req.count_required);
                    if (idx < building_count) {
                        printf("    - Building (%s, count=%u)\n", buildings[idx].name.c_str(), ct);
                    } else {
                        printf("    - Building (idx=<invalid index %u>, count=%u)\n", idx, ct);
                    }
                    break;
                }
                default:
                    printf("    - <unknown requirement type %u>\n", static_cast<u32>(req.type));
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
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");
    CityFlagData::load_static_data("../game_config.city_flags");
    SmallWonderData::load_static_data("../game_config.wonders_small");

    print_wonder_small_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
