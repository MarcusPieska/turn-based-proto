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

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_building_data () {
    u16 building_count = BuildingData::get_building_data_count();
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();

    u16 tech_count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();

    u16 resource_count = ResourceData::get_resource_data_count();
    const ResourceTypeStats* resources = ResourceData::get_resource_data_array();

    u16 flag_count = CityFlagData::get_flag_count();
    const CityFlagStats* flags = CityFlagData::get_flag_data_array();

    printf("\n=======================================================\n");
    printf("BUILDINGS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < building_count; ++i) {
        const BuildingTypeStats& b = buildings[i];

        printf("%s\n", b.name.c_str());
        printf("  Cost: %u\n", static_cast<u32>(b.cost));

        u16 tech_idx = b.tech_prereq_idx.get_idx();
        if (tech_idx < tech_count) {
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[tech_idx].name.c_str(), tech_idx);
        } else {
            printf("  Tech Prerequisite: <invalid index %u>\n", tech_idx);
        }

        bool has_reqs = false;
        for (u32 r = 0; r < MAX_BUILDING_REQS; ++r) {
            const BuildingRequirement& req = b.requirements[r];
            if (req.type == BUILDING_REQ_NONE) {
                continue;
            }

            if (!has_reqs) {
                printf("  Requirements:\n");
                has_reqs = true;
            }

            switch (req.type) {
                case BUILDING_REQ_FLAG: {
                    u32 idx = static_cast<u32>(req.data.flag_req.flag_idx);
                    if (idx < flag_count) {
                        printf("    - Flag (%s)\n", flags[idx].name.c_str());
                    } else {
                        printf("    - Flag (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case BUILDING_REQ_RESOURCE: {
                    u32 idx = static_cast<u32>(req.data.resource_req.resource_idx);
                    if (idx < resource_count) {
                        printf("    - Resource (%s)\n", resources[idx].name.c_str());
                    } else {
                        printf("    - Resource (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case BUILDING_REQ_BUILDING: {
                    u32 idx = static_cast<u32>(req.data.building_req.building_idx);
                    u32 ct = static_cast<u32>(req.data.building_req.count_required);
                    if (idx < building_count) {
                        printf("    - Building (%s, count=%u)\n", buildings[idx].name.c_str(), ct);
                    } else {
                        printf("    - Building (<invalid index %u>, count=%u)\n", idx, ct);
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
    CityFlagData::load_static_data("../game_config.city_flags");
    BuildingData::load_static_data("../game_config.buildings");

    print_building_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

