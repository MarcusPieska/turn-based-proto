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

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_civ_trait_data () {
    u16 trait_count = CivTraitData::get_civ_trait_count();
    const CivTraitStats* traits = CivTraitData::get_civ_trait_data_array();

    u16 building_count = BuildingData::get_building_data_count();
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();

    printf("\n=======================================================\n");
    printf("CIV TRAITS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < trait_count; ++i) {
        const CivTraitStats& trait = traits[i];

        printf("Trait %s buildings:\n", trait.name.c_str());
        for (u16 j = 0; j < building_count; ++j) {
            if (trait.buildings->get_bit(j) == 1) {
                printf("  %s (idx=%u)\n", buildings[j].name.c_str(), j);
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

    print_civ_trait_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
