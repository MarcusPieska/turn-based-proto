//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef WONDER_SMALL_DATA_H
#define WONDER_SMALL_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - Forward declarations and constants -
//================================================================================================================================

class BitArrayCL;

static const u32 MAX_WONDER_SMALL_REQS = 4;

//================================================================================================================================
//=> - Requirement enums and structs -
//================================================================================================================================

enum SmallWonderReqType : u16 {
    WONDER_REQ_NONE = 0,
    WONDER_REQ_FLAG = 1,
    WONDER_REQ_RESOURCE = 2,
    WONDER_REQ_BUILDING = 3
};


struct SmallWonderReqFlag {
    u16 flag_idx;
};

struct SmallWonderReqResource {
    u16 resource_idx;
};

struct SmallWonderReqBuilding {
    u16 building_idx;
    u16 count_required;
};

struct SmallWonderRequirement {
    SmallWonderReqType type;
    union {
        SmallWonderReqFlag flag_req;
        SmallWonderReqResource resource_req;
        SmallWonderReqBuilding building_req;
    } data;
};

//================================================================================================================================
//=> - SmallWonderTypeStats struct -
//================================================================================================================================

struct SmallWonderTypeStats {
    std::string name;
    u32 cost;
    u16 tech_prereq_idx;
    SmallWonderRequirement requirements[MAX_WONDER_SMALL_REQS];
};

//================================================================================================================================
//=> - SmallWonderData class -
//================================================================================================================================

class SmallWonderData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 get_wonder_data_count ();
    static const SmallWonderTypeStats* get_wonder_data_array ();

private:
    static u16 validate_and_count (const std::string& filename);
    static void parse_and_allocate (const std::string& filename);

    SmallWonderData () = delete;
    SmallWonderData (const SmallWonderData& other) = delete;
    SmallWonderData (SmallWonderData&& other) = delete;
};

#endif // WONDER_SMALL_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

