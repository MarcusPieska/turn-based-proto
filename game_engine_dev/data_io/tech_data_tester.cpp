//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tech_data.h"

//================================================================================================================================
//=> - Tests -
//================================================================================================================================

void print_tech_data () {
    TechData::print_content();
}

void print_tech_tree () {
    u16 count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();
    int* researched = new int[count];
    int* prev_researched = new int[count];
    for (u16 i = 0; i < count; ++i) {
        researched[i] = 0;
    }
    researched[0] = 1;
    while (true) {
        int flips_this_pass = 0;
        printf("\n=======================================================\n");
        prev_researched = (int*)memcpy(prev_researched, researched, count * sizeof(int));
        for (u16 i = 0; i < count; ++i) {
            if (prev_researched[i] != 0) {
                continue;
            }
            const TechTypeStats& tech = techs[i];
            bool can_research = true;
            for (u32 p = 0; p < MAX_TECHS_PER_ENTITY; ++p) {
                u16 prereq_idx = tech.tech_indices.indices[p];
                if (prev_researched[prereq_idx] == 0) {
                    can_research = false;
                    break;
                }
            }
            if (can_research) {
                printf("Tech: %s\n", tech.name.c_str());
                researched[i] = 1;
                flips_this_pass++;
            }
        }

        if (flips_this_pass == 0) {
            break; 
        }
    }
    int missing_techs = 0;
    for (u16 i = 0; i < count; ++i) {
        if (researched[i] == 0) {
            missing_techs++;
            printf("Missing tech: %s\n", techs[i].name.c_str());
        }
    }
    if (missing_techs > 0) {
        printf("Missing %d techs\n", missing_techs);
    } else {
        printf("All techs have been researched\n");
    }
    delete[] researched;
    delete[] prev_researched;
}

//================================================================================================================================
//=> - Public functions -
//================================================================================================================================

int main (int argc, char* argv[]) {
    TechData::load_static_data("../game_config.techs");

    print_tech_data();
    print_tech_tree();

    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
