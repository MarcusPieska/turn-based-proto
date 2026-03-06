//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILDING_DATA_H
#define BUILDING_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - BuildingTypeStats struct -
//================================================================================================================================

class BitArrayCL;

typedef struct BuildingTypeStats {
    std::string name;
    u32 cost;
    u16 tech_prereq_index;
    ResourceIndices resource_indices;
    EffectIndices effect_indices;
} BuildingTypeStats;

//================================================================================================================================
//=> - BuildingData class -
//================================================================================================================================

class BuildingData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_tech_index (const std::string& tech_name);
    static u16 find_resource_index (const std::string& resource_name);
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
