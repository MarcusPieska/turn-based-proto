//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <cstdio>

#include "bit_array.h"
#include "unit_data.h"
#include "unit_vector.h"
#include "building_vector.h"
#include "unit_assessor.h"

//================================================================================================================================
//=> - UnitAssessor implementation -
//================================================================================================================================

BuildableUnits* UnitAssessor::assess(
    const BitArrayCL* techs,
    const BuiltBuildings* buildings,
    const BitArrayCL* resources,
    const BitArrayCL* flags
) {
    const UnitTypeStats* units = UnitData::get_unit_data_array();
    u16 unit_count = UnitData::get_unit_data_count();
    BuildableUnits* result = new BuildableUnits();

    for (u16 i = 0; i < unit_count; ++i) {
        const UnitTypeStats& u = units[i];
        if (techs->get_bit(u.tech_prereq_idx.get_idx()) != 1) {
            continue;
        }

        bool found_breaking_requirement = false;
        for (u32 r = 0; r < MAX_UNIT_REQS; ++r) {
            const UnitRequirement& req = u.requirements[r];
            if (req.type == UNIT_REQ_NONE) {
                break;
            }

            switch (req.type) {
                case UNIT_REQ_FLAG:
                    if (flags->get_bit(req.data.flag_req.flag_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;
                case UNIT_REQ_RESOURCE:
                    if (resources->get_bit(req.data.resource_req.resource_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;
                case UNIT_REQ_BUILDING:
                    if (buildings->has_been_built(req.data.building_req.building_idx) != 1) {
                        found_breaking_requirement = true;
                    }
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