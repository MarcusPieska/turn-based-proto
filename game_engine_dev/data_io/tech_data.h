//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef TECH_DATA_H
#define TECH_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "tech_data_types.h"

//================================================================================================================================
//=> - BuildingTypeStats struct -
//================================================================================================================================

class BitArrayCL;

typedef struct TechTypeStats {
    std::string name;
    u32 cost;
    TechIndices tech_indices;
} TechTypeStats;

//================================================================================================================================
//=> - TechData class -
//================================================================================================================================

class TechData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static TechIdx find_tech_index (const std::string& tech_name);
    static u16 get_tech_data_count ();
    static const TechTypeStats* get_tech_data_array ();

private:
    static u16 validate_and_count (const std::string& filename);
    static void parse_and_allocate (const std::string& filename);

    TechData () = delete;
    TechData (const TechData& other) = delete;
    TechData (TechData&& other) = delete;
};

#endif // TECH_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
