//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "tech_data.h"
#include "resource_data.h"
#include "city_flags.h"
#include "building_data.h"
#include "civ_trait_data.h"
#include "civ_data.h"

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_civ_data () {
    u16 civ_count = CivData::get_civ_data_count();
    const CivStats* civs = CivData::get_civ_data_array();

    u16 trait_count = CivTraitData::get_civ_trait_count();
    const CivTraitStats* traits = CivTraitData::get_civ_trait_data_array();

    printf("\n=======================================================\n");
    printf("CIVS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < civ_count; ++i) {
        const CivStats& civ = civs[i];

        printf("Civ %s traits:\n", civ.name.c_str());
        for (u16 j = 0; j < MAX_TRAITS_PER_CIV; ++j) {
            u16 trait_idx = civ.trait_indices.indices[j];
            if (trait_idx == 0) {
                break;
            }
            if (trait_idx < trait_count) {
                printf("  %s (idx=%u)\n", traits[trait_idx].name.c_str(), trait_idx);
            }
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
    CivTraitData::load_static_data("../game_config.civ_traits");
    CivData::load_static_data("../game_config.civs");

    print_civ_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
