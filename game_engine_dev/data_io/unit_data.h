//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef UNIT_DATA_H
#define UNIT_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - UnitTypeStats struct -
//================================================================================================================================

class BitArrayCL;

typedef struct UnitTypeStats {
    std::string name;
    u32 cost;
    u16 attack;
    u16 defense;
    u16 movement_speed;
    u16 tech_prereq_index;
    ResourceIndices resource_indices;
} UnitTypeStats;

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
