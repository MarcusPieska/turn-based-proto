//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TILE_H
#define MAP_TILE_H

#include <cstdint>

struct Point {
    int32_t x;
    int32_t y;
    Point () : x(0), y(0) {}
    Point (int32_t x_val, int32_t y_val) : x(x_val), y(y_val) {}
};

struct Deltas {
    int16_t top;
    int16_t right;
    int16_t bottom;
    int16_t left;
    Deltas () : top(0), right(0), bottom(0), left(0) {}
};

struct MapTile {
    Point top;
    Point right;
    Point bottom;
    Point left;
    int32_t z;
    int32_t lowest;
    int32_t highest;
    Deltas deltas;
    MapTile () : z(0), lowest(0), highest(0) {}
    MapTile (Point top_pt, Point right_pt, Point bottom_pt, Point left_pt) : top(top_pt), right(right_pt), bottom(bottom_pt), left(left_pt), z(0), lowest(0), highest(0) {}
};

#endif // MAP_TILE_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

