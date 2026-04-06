//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef CIV_TRAIT_DATA_H
#define CIV_TRAIT_DATA_H

#include <string>
#include <iosfwd>
#include <cstdint>

#include "game_primitives.h"
#include "bit_array.h"

//================================================================================================================================
//=> - Forward declarations -
//================================================================================================================================

class BitArrayCL;

//================================================================================================================================
//=> - CivTraitStats struct -
//================================================================================================================================

typedef struct CivTraitStats {
    std::string name;
    BitArrayCL buildings;
} CivTraitStats;

//================================================================================================================================
//=> - CivTraitData class -
//================================================================================================================================

class CivTraitData {
public:
    static void load_static_data (const std::string& filename);
    static void print_content ();
    static u16 find_civ_trait_index (const std::string& trait_name);
    static u16 get_civ_trait_count ();
    static const CivTraitStats* get_civ_trait_data_array ();

private:
    static void parse_and_allocate (const std::string& filename);

    CivTraitData () = delete;
    CivTraitData (const CivTraitData& other) = delete;
    CivTraitData (CivTraitData&& other) = delete;
};

#endif // CIV_TRAIT_DATA_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
