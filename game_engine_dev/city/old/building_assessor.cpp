//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <string>

#include "bit_array.h"
#include "building_data.h"
#include "building_vector.h"

#include "building_assessor.h"

//================================================================================================================================
//=> - BuildingAssessor implementation -
//================================================================================================================================

BuildableBuildings* BuildingAssessor::assess (
    const BitArrayCL* techs,
    const BuiltBuildings* buildings,
    const BitArrayCL* resources,
    const BitArrayCL* flags)
{
    const BuildingTypeStats* building_data = BuildingData::get_building_data_array();
    u16 building_count = BuildingData::get_building_data_count();
    BuildableBuildings* result = new BuildableBuildings();
    
    for (u16 i = 0; i < building_count; ++i) {
        const BuildingTypeStats& b = building_data[i];
        
        if (buildings != nullptr && buildings->has_been_built(i)) {
            continue;
        }
        
        if (techs->get_bit(b.tech_prereq_idx.get_idx()) != 1) {
            continue;
        }

        bool found_breaking_requirement = false;
        for (u32 r = 0; r < MAX_BUILDING_REQS; ++r) {
            const BuildingRequirement& req = b.requirements[r];
            if (req.type == BUILDING_REQ_NONE) {
                break;
            }

            switch (req.type) {
                case BUILDING_REQ_FLAG:
                    if (flags->get_bit(req.data.flag_req.flag_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case BUILDING_REQ_RESOURCE:
                    if (resources->get_bit(req.data.resource_req.resource_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case BUILDING_REQ_BUILDING:
                    if (buildings == nullptr || !buildings->has_been_built(req.data.building_req.building_idx)) {
                        found_breaking_requirement = true;
                    }
                    // TODO: Count checking not implemented - assumes at least 1 is sufficient
                    break;

                default:
                    found_breaking_requirement = true;
                    break;
            }

            if (found_breaking_requirement) {
                break;
            }
        }
        
        if (found_breaking_requirement) {
            continue;
        }
        
        result->set_buildable(i);
    }
    
    return result;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
