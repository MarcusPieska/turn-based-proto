//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdint>
#include <string>

#include "bit_array.h"
#include "building_vector.h"
#include "small_wonder_data.h"
#include "small_wonder_vector.h"

#include "small_wonder_assessor.h"

//================================================================================================================================
//=> - SmallWonderAssessor implementation -
//================================================================================================================================

BuildableSmallWonders* SmallWonderAssessor::assess (
    const BitArrayCL* techs,
    const BuiltBuildings* buildings,
    const BitArrayCL* resources,
    const BitArrayCL* flags,
    const BuiltSmallWonders* built)
{
    const SmallWonderTypeStats* wonders = SmallWonderData::get_small_wonder_data_array();
    u16 wonder_count = SmallWonderData::get_small_wonder_data_count();
    BuildableSmallWonders* result = new BuildableSmallWonders();
    for (u16 i = 0; i < wonder_count; ++i) {
        const SmallWonderTypeStats& w = wonders[i];
        if (built->has_been_built(i)) {
            continue;
        }
        if (techs->get_bit(w.tech_prereq_idx.get_idx()) != 1) {
            continue;
        }

        bool found_breaking_requirement = false;
        for (u32 r = 0; r < MAX_WONDER_SMALL_REQS; ++r) {
            const SmallWonderRequirement& req = w.requirements[r];
            if (req.type == SMALL_WONDER_REQ_NONE) {
                break;
            }

            switch (req.type) {
                case SMALL_WONDER_REQ_FLAG:
                    if (flags->get_bit(req.data.flag_req.flag_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case SMALL_WONDER_REQ_RESOURCE:
                    if (resources->get_bit(req.data.resource_req.resource_idx) != 1) {
                        found_breaking_requirement = true;
                    }
                    break;

                case SMALL_WONDER_REQ_BUILDING:
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
