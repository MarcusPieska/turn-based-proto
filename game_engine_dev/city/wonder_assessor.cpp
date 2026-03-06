//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <string>

#include "bit_array.h"
#include "wonder_data.h"
#include "wonder_vector.h"

#include "wonder_assessor.h"

//================================================================================================================================
//=> - WonderAssessor implementation -
//================================================================================================================================

WonderBuildableVector* WonderAssessor::assess (
    const BitArrayCL* techs,
    const BitArrayCL* buildings,
    const BitArrayCL* resources,
    const BitArrayCL* flags)
{
    const WonderTypeStats* wonders = WonderData::get_wonder_data_array();
    u16 wonder_count = WonderData::get_wonder_data_count();
    WonderBuildableVector* result = new WonderBuildableVector();
    for (u16 i = 0; i < wonder_count; ++i) {
        const WonderTypeStats& w = wonders[i];
        if (WondersBuiltVector::has_been_built(i)) {
            continue;
        }
        if (techs->get_bit(static_cast<int>(w.tech_prereq_idx)) != 1) {
            continue;
        }

        bool found_breaking_requirement = false;
        for (u32 r = 0; r < MAX_WONDER_REQS; ++r) {
            const WonderRequirement& req = w.requirements[r];
            if (req.type == WONDER_REQ_NONE) {
                continue;
            }

            switch (req.type) {
                case WONDER_REQ_FLAG:
                    if (flags->get_bit(static_cast<int>(req.data.flag_req.flag_idx)) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case WONDER_REQ_RESOURCE:
                    if (resources->get_bit(static_cast<int>(req.data.resource_req.resource_idx)) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case WONDER_REQ_BUILDING:
                    if (buildings->get_bit(static_cast<int>(req.data.building_req.building_idx)) != 1) {
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
