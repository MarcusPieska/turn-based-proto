//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tech_data.h"
#include "resource_data.h"

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_resource_data () {
    u16 resource_count = ResourceData::get_resource_data_count();
    const ResourceTypeStats* resources = ResourceData::get_resource_data_array();

    u16 tech_count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();

    printf("\n=======================================================\n");
    printf("RESOURCES\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < resource_count; ++i) {
        const ResourceTypeStats& r = resources[i];

        printf("%s\n", r.name.c_str());
        printf("  Yields: Food=%u, Shields=%u, Commerce=%u\n",
               static_cast<u32>(r.bonus_food),
               static_cast<u32>(r.bonus_shields),
               static_cast<u32>(r.bonus_commerce));

        if (r.tech_prereq_index < tech_count) {
            u16 idx = r.tech_prereq_index;
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[idx].name.c_str(), static_cast<u32>(idx));
        } else if (r.tech_prereq_index == 0) {
            printf("  Tech Prerequisite: None\n");
        } else {
            printf("  Tech Prerequisite: <invalid index %u>\n", static_cast<u32>(r.tech_prereq_index));
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

    print_resource_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================

