//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "city_flags.h"
#include "tech_data.h"
#include "resource_data.h"
#include "building_data.h"
#include "unit_data.h"
#include "resource_data_types.h"

//================================================================================================================================
//=> - Print functions -
//================================================================================================================================

void print_unit_data () {
    u16 unit_count = UnitData::get_unit_data_count();
    const UnitTypeStats* units = UnitData::get_unit_data_array();
    
    u16 tech_count = TechData::get_tech_data_count();
    const TechTypeStats* techs = TechData::get_tech_data_array();
    
    u16 resource_count = ResourceData::get_resource_data_count();
    const ResourceTypeStats* resources = ResourceData::get_resource_data_array();

    u16 flag_count = CityFlagData::get_flag_count();
    const CityFlagStats* flags = CityFlagData::get_flag_data_array();

    u16 building_count = BuildingData::get_building_data_count();
    const BuildingTypeStats* buildings = BuildingData::get_building_data_array();

    printf("\n=======================================================\n");
    printf("UNITS\n");
    printf("=======================================================\n\n");

    for (u16 i = 0; i < unit_count; ++i) {
        const UnitTypeStats& unit = units[i];
        
        printf("%s\n", unit.name.c_str());
        printf("  Cost: %u\n", static_cast<u32>(unit.cost));
        printf("  Attack: %u\n", static_cast<u32>(unit.attack));
        printf("  Defense: %u\n", static_cast<u32>(unit.defense));
        printf("  Movement Speed: %u\n", static_cast<u32>(unit.movement_speed));
        
        if (unit.tech_prereq_idx.get_idx() < tech_count) {
            u32 idx = unit.tech_prereq_idx.get_idx();
            printf("  Tech Prerequisite: %s (idx=%u)\n", techs[idx].name.c_str(), idx);
        } else {
            u32 idx = unit.tech_prereq_idx.get_idx();
            printf("  Tech Prerequisite: <invalid index %u>\n", idx);
        }
        
        bool has_reqs = false;
        for (u32 r = 0; r < MAX_UNIT_REQS; ++r) {
            const UnitRequirement& req = unit.requirements[r];
            if (req.type == UNIT_REQ_NONE) {
                continue;
            }

            if (!has_reqs) {
                printf("  Requirements:\n");
                has_reqs = true;
            }

            switch (req.type) {
                case UNIT_REQ_FLAG: {
                    u32 idx = static_cast<u32>(req.data.flag_req.flag_idx);
                    if (idx < flag_count) {
                        printf("    - Flag (%s)\n", flags[idx].name.c_str());
                    } else {
                        printf("    - Flag (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case UNIT_REQ_RESOURCE: {
                    u32 idx = static_cast<u32>(req.data.resource_req.resource_idx);
                    if (idx < resource_count) {
                        printf("    - Resource (%s)\n", resources[idx].name.c_str());
                    } else {
                        printf("    - Resource (<invalid index %u>)\n", idx);
                    }
                    break;
                }
                case UNIT_REQ_BUILDING: {
                    u32 idx = static_cast<u32>(req.data.building_req.building_idx);
                    u32 ct  = static_cast<u32>(req.data.building_req.count_required);
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
    CityFlagData::load_static_data("../game_config.city_flags");
    TechData::load_static_data("../game_config.techs");
    ResourceData::load_static_data("../game_config.resources");
    BuildingData::load_static_data("../game_config.buildings");
    UnitData::load_static_data("../game_config.units");

    print_unit_data();
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
