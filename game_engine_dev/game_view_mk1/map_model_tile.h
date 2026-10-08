//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_MODEL_TILE_H
#define MAP_MODEL_TILE_H

#include <cstdint>

#include "map_types.h"

struct MapModelTile {
    Point top;
    Point right;
    Point bottom;
    Point left;
    int32_t z;
    
    MapModelTile () : z(0) {}
    MapModelTile (Point top_pt, Point right_pt, Point bottom_pt, Point left_pt) : 
        top(top_pt), 
        right(right_pt), 
        bottom(bottom_pt), 
        left(left_pt), 
        z(0) {
    }
};

#endif // MAP_MODEL_TILE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

