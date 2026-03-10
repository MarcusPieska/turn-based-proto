//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_DATA_H
#define BUILDING_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "tech_data_types.h"
#include "resource_data_types.h"

//================================================================================================================================
//=> - Forward declarations and constants -
//================================================================================================================================

class BitArrayCL;

static const u32 MAX_BUILDING_REQS = 4;

//================================================================================================================================
//=> - Requirement enums and structs -
//================================================================================================================================

enum BuildingReqType : u16 {
    BUILDING_REQ_NONE = 0,
    BUILDING_REQ_FLAG = 1,
    BUILDING_REQ_RESOURCE = 2,
    BUILDING_REQ_BUILDING = 3
};

struct BuildingReqFlag {
    u16 flag_idx;
};

struct BuildingReqResource {
    u16 resource_idx;
};

struct BuildingReqBuilding {
    u16 building_idx;
    u16 count_required;
};

struct BuildingRequirement {
    BuildingReqType type;
    union {
        BuildingReqFlag flag_req;
        BuildingReqResource resource_req;
        BuildingReqBuilding building_req;
    } data;
};

//================================================================================================================================
//=> - BuildingTypeStats struct -
//================================================================================================================================

typedef struct BuildingTypeStats {
    std::string name;
    u32 cost;
    TechIdx tech_prereq_idx;
    BuildingRequirement requirements[MAX_BUILDING_REQS];
    EffectIndices effect_indices;
} BuildingTypeStats;

//================================================================================================================================
//=> - BuildingData class -
//================================================================================================================================

class BuildingData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_building_index (const std::string& building_name);
    static u16 get_building_data_count ();
    static const BuildingTypeStats* get_building_data_array ();

private:
    static u16 validate_and_count (const std::string& filename);
    static void parse_and_allocate (const std::string& filename);

    BuildingData () = delete;
    BuildingData (const BuildingData& other) = delete;
    BuildingData (BuildingData&& other) = delete;
};

#endif // BUILDING_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
