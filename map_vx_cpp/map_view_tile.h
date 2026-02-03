//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_VIEW_TILE_H
#define MAP_VIEW_TILE_H

#include <cstdint>

#include "map_types.h"
#include "dat15_io.h"

typedef struct MapViewTileFlags {
    uint8_t mountain : 1;    
    uint8_t river : 1;  
    uint8_t farm_lvl : 3;
    uint8_t misc_items : 1; 
    uint8_t padding : 2;  

    MapViewTileFlags () : mountain(0), river(0), misc_items(0), padding(0) {}
} MapViewTileFlags;

typedef struct MapViewTile {
    int32_t lowest;
    int32_t highest;
    Deltas deltas;
    Dat15Reader* tex_read;
    MapViewTileFlags flags;
    
    MapViewTile () : lowest(0), highest(0), deltas(), tex_read(nullptr) {}
    MapViewTile (int32_t lowest_val, int32_t highest_val) : 
        lowest(lowest_val), 
        highest(highest_val),
        deltas(),
        tex_read(nullptr),
        flags() {  
    }
} MapViewTile;

#endif // MAP_VIEW_TILE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

