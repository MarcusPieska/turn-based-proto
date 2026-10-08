//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DIA_CFG_H
#define DIA_CFG_H

#include <cstdint>

//================================================================================================================================
//=> - Diamond tile macros -
//================================================================================================================================

#define SUBTILE_DIV      10
#define TILE_SPACE_X     100
#define TILE_DRAW_W      100
#define TILE_H           50
#define TILE_HALF_W      (TILE_SPACE_X / 2)
#define TILE_HALF_H      (TILE_H / 2)
#define GRID_N           20
#define TERRAIN_ID_MAX   16

#define CORNER_PULL_PCT  50
#define CORNER_PULL_SUB  4

#define STRIPE_PULL_PCT  100
#define STRIPE_PULL_SUB  5
#define EDGE_MAX_DEV     2

struct Vx {
    int32_t x;
    int32_t y;

    Vx () : x(0), y(0) {}
    Vx (int32_t xx, int32_t yy) : x(xx), y(yy) {}
};

struct Rgb {
    uint8_t r, g, b;
    Rgb () : r(0), g(0), b(0) {}
    Rgb (uint8_t rr, uint8_t gg, uint8_t bb) : r(rr), g(gg), b(bb) {}
};

enum EdgeKind {
    EDGE_NE = 0,                                                                 // shared (r,c)-(r,c+1): top->right
    EDGE_SE = 1                                                                  // shared (r,c)-(r+1,c): right->bottom
};

#endif // DIA_CFG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
