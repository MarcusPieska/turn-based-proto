//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_DATA_H
#define CIV_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"

//================================================================================================================================
//=> - CivTraitIndices struct -
//================================================================================================================================

#define MAX_TRAITS_PER_CIV 4

typedef struct CivTraitIndices {
    u16 indices[MAX_TRAITS_PER_CIV];
} CivTraitIndices;

//================================================================================================================================
//=> - CivStats struct -
//================================================================================================================================

typedef struct CivStats {
    std::string name;
    CivTraitIndices trait_indices;
} CivStats;

//================================================================================================================================
//=> - CivData class -
//================================================================================================================================

class CivData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_civ_index (const std::string& civ_name);
    static u16 get_civ_data_count ();
    static const CivStats* get_civ_data_array ();

private:
    static void parse_and_allocate (const std::string& filename);

    CivData () = delete;
    CivData (const CivData& other) = delete;
    CivData (CivData&& other) = delete;
};

#endif // CIV_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
