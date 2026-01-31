//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_VIEW_TILE_H
#define MAP_VIEW_TILE_H

#include <cstdint>

#include "map_types.h"
#include "dat15_io.h"

struct MapViewTile {
    int32_t lowest;
    int32_t highest;
    Deltas deltas;
    Dat15Reader* tex_read;
    
    MapViewTile () : lowest(0), highest(0), deltas(), tex_read(nullptr) {}
    MapViewTile (int32_t lowest_val, int32_t highest_val) : 
        lowest(lowest_val), 
        highest(highest_val),
        deltas(),
        tex_read(nullptr) {  
    }
};

#endif // MAP_VIEW_TILE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

