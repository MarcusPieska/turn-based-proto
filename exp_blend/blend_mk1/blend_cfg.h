//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef BLEND_CFG_H
#define BLEND_CFG_H

#include <cstdint>

//================================================================================================================================
//=> - Grid and tile macros -
//================================================================================================================================

#define SUBTILE_DIV      10
#define TILE_PX          100
#define SUBTILE_PX       (TILE_PX / SUBTILE_DIV)
#define TERRAIN_N        4
#define TERRAIN_ID_MAX   16
#define CORNER_PULL_PCT  70
#define CORNER_PULL_SUB  2
#define EDGE_MAX_DEV     2

//================================================================================================================================
//=> - Terrain ids and flat colors -
//================================================================================================================================

enum TerrainId {
    TER_DESERT     = 0,                                                          // pale yellow
    TER_PLAINS     = 1,                                                          // wheat yellow
    TER_GRASSLANDS = 2,                                                          // green
    TER_BLACKSOIL  = 3                                                           // deeper darker green
};

struct Rgb {
    uint8_t r;
    uint8_t g;
    uint8_t b;

    Rgb () : r(0), g(0), b(0) {}
    Rgb (uint8_t rr, uint8_t gg, uint8_t bb) : r(rr), g(gg), b(bb) {}
};

static const Rgb TER_COL[TERRAIN_N] = {
    Rgb (238, 214, 120),
    Rgb (210, 170,  70),
    Rgb ( 70, 150,  65),
    Rgb ( 35,  85,  48)
};

struct Vx {
    int32_t x;
    int32_t y;

    Vx () : x(0), y(0) {}
    Vx (int32_t xx, int32_t yy) : x(xx), y(yy) {}
};

enum EdgeKind {
    EDGE_H = 0,
    EDGE_V = 1
};

enum ClipStyle {
    CLIP_LEGACY = 0,                                                             // half-plane carve along threads
    CLIP_FLOOD  = 1                                                              // seams + flood from tile centers
};

#endif // BLEND_CFG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
