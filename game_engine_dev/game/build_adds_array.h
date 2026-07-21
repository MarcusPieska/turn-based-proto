//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BUILD_ADDS_ARRAY_H
#define BUILD_ADDS_ARRAY_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Build add types -
//================================================================================================================================

// Note that units have their own u16 link in GameTileSimple; no need for a BUILD_ADD_UNIT type
enum BuildAddType : u16 {
    BUILD_ADD_STD = 0, // Interpret the u16 as a straight bitmap with local information, not as key to some other array
    BUILD_ADD_CITY = 1,
    BUILD_ADD_MINE = 2,
    BUILD_ADD_PLANTATION = 3,
    BUILD_ADD_FORT = 4,
    BUILD_ADD_SHIPYARD = 5,
    BUILD_ADD_OUTPOST = 6,
    BUILD_ADD_TRADE_POST = 7,
    BUILD_ADD_MONASTERY = 8
};

//================================================================================================================================
//=> - BuildAddItem -
//================================================================================================================================

struct BuildAddItem {
    u8 x_offset;
    u8 y_offset;
    u16 build_add_type;
    u16 build_add_idx;
    u16 resource_idx;
    u16 next_add_idx;
};

#endif // BUILD_ADDS_ARRAY_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
