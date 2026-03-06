//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef RESOURCE_DATA_H
#define RESOURCE_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingTypeStats struct -
//================================================================================================================================

class BitArrayCL;

typedef struct ResourceTypeStats {
    std::string name;
    u16 bonus_food;
    u16 bonus_shields;
    u16 bonus_commerce;
    u16 tech_prereq_index;
} ResourceTypeStats;

//================================================================================================================================
//=> - ResourceData class -
//================================================================================================================================

class ResourceData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_resource_index (const std::string& resource_name);
    static u16 get_resource_data_count ();
    static const ResourceTypeStats* get_resource_data_array ();

private:
    static u16 validate_and_count (const std::string& filename);
    static void parse_and_allocate (const std::string& filename);

    ResourceData () = delete;
    ResourceData (const ResourceData& other) = delete;
    ResourceData (ResourceData&& other) = delete;
};

#endif // RESOURCE_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
