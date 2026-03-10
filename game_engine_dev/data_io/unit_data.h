//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_DATA_H
#define UNIT_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "tech_data_types.h"
#include "resource_data_types.h"

//================================================================================================================================
//=> - UnitTypeStats struct -
//================================================================================================================================

class BitArrayCL;

static const u32 MAX_UNIT_REQS = MAX_RESOURCES_PER_ENTITY;

enum UnitReqType : u16 {
    UNIT_REQ_NONE = 0,
    UNIT_REQ_FLAG = 1,
    UNIT_REQ_RESOURCE = 2,
    UNIT_REQ_BUILDING = 3
};

struct UnitReqFlag {
    u16 flag_idx;
};

struct UnitReqResource {
    u16 resource_idx;
};

struct UnitReqBuilding {
    u16 building_idx;
    u16 count_required;
};

struct UnitRequirement {
    UnitReqType type;
    union {
        UnitReqFlag flag_req;
        UnitReqResource resource_req;
        UnitReqBuilding building_req;
    } data;
};

struct UnitTypeStats {
    std::string name;
    u32 cost;
    u16 attack;
    u16 defense;
    u16 movement_speed;
    TechIdx tech_prereq_idx;
    UnitRequirement requirements[MAX_UNIT_REQS];
};

//================================================================================================================================
//=> - UnitData class -
//================================================================================================================================

class UnitData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_unit_index (const std::string& unit_name);
    static u16 get_unit_data_count ();
    static const UnitTypeStats* get_unit_data_array ();

private:
    static u16 validate_and_count (const std::string& filename);
    static void parse_and_allocate (const std::string& filename);

    UnitData () = delete;
    UnitData (const UnitData& other) = delete;
    UnitData (UnitData&& other) = delete;
};

#endif // UNIT_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
