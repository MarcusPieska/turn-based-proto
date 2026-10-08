//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef MAP_TYPES_H
#define MAP_TYPES_H

#include <cstdint>

typedef struct Point {
    int32_t x;
    int32_t y;

    Point () : x(0), y(0) {}
    Point (int32_t x, int32_t y) : x(x), y(y) {}
} Point;

typedef struct Size {
    int32_t width;
    int32_t height;

    Size () : width(0), height(0) {}
    Size (int32_t w, int32_t h) : width(w), height(h) {}
} Size;

typedef struct Rect {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;

    Rect () : x(0), y(0), width(0), height(0) {}
    Rect (int32_t x, int32_t y, int32_t w, int32_t h) : x(x), y(y), width(w), height(h) {}
} Rect;

typedef struct Deltas {
    int16_t top;
    int16_t right;
    int16_t bottom;
    int16_t left;

    Deltas () : top(0), right(0), bottom(0), left(0) {}
} Deltas;

typedef struct NearTiles {
    Point n;
    Point ne;
    Point e;
    Point se;
    Point s;
    Point sw;
    Point w;
    Point nw;

    NearTiles () : n(-1, -1), ne(-1, -1), e(-1, -1), se(-1, -1), s(-1, -1), sw(-1, -1), w(-1, -1), nw(-1, -1) {}
} NearTiles;

#endif // MAP_TYPES_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================

