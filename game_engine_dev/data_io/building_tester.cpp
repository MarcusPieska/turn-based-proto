//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tech_data.h"
#include "building_data.h"
#include "resource_data.h"

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

    printf("\n=======================================================\n");
    printf("BUILDINGS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < building_count; ++i) {
        const BuildingTypeStats& b = buildings[i];

        printf("%s\n", b.name.c_str());
        printf("  Cost: %u\n", static_cast<u32>(b.cost));

        if (b.tech_prereq_index < tech_count) {
            u16 idx = b.tech_prereq_index;
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[idx].name.c_str(), static_cast<u32>(idx));
        } else {
            printf("  Tech Prerequisite: <invalid index %u>\n", static_cast<u32>(b.tech_prereq_index));
        }

        bool has_res = false;
        for (u32 r = 0; r < MAX_RESOURCES_PER_ENTITY; ++r) {
            u16 res_idx = b.resource_indices.indices[r];
            if (res_idx == 0) {
                continue;
            }

            if (!has_res) {
                printf("  Resources:\n");
                has_res = true;
            }

            u32 idx = static_cast<u32>(res_idx);
            if (idx < resource_count) {
                printf("    - %s (idx=%u)\n", resources[idx].name.c_str(), idx);
            } else {
                printf("    - <invalid resource index %u>\n", idx);
            }
        }

        if (!has_res) {
            printf("  Resources: None\n");
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

    print_building_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

