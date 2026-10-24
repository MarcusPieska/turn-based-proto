//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef DIA_CFG_H
#define DIA_CFG_H

#include <cstdint>

//================================================================================================================================
//=> - Diamond tile and corner catalog -
//================================================================================================================================

#define SUBTILE_DIV 7
#define TILE_SPACE_X 100
#define TILE_DRAW_W 100
#define TILE_H 50
#define TILE_HALF_W (TILE_SPACE_X / 2)
#define TILE_HALF_H (TILE_H / 2)
#define PULL_UX (TILE_HALF_W / SUBTILE_DIV)
#define PULL_UY (TILE_HALF_H / SUBTILE_DIV)
#define TERRAIN_N 4
#define TERRAIN_ID_MAX 16
#define CORNER_PULL_N 2
#define SOLO_DUP_PCT 30

static const int CORNER_PULL_U[CORNER_PULL_N] = { 2, 3 };

//================================================================================================================================
//=> - Terrain ids and flat colors -
//================================================================================================================================

enum TerrainId {
    TER_DESERT = 0, // pale yellow
    TER_PLAINS = 1, // wheat yellow
    TER_GRASSLANDS = 2, // green
    TER_BLACKSOIL = 3 // deeper darker green
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
    Rgb (210, 170, 70),
    Rgb (70, 150, 65),
    Rgb (35, 85, 48)
};

struct Vx {
    int32_t x;
    int32_t y;

    Vx () : x(0), y(0) {}
    Vx (int32_t xx, int32_t yy) : x(xx), y(yy) {}
};

enum EdgeKind {
    EDGE_NE = 0, // shared (r,c)-(r,c+1): top->right
    EDGE_SE = 1, // shared (r,c)-(r+1,c): right->bottom
    EDGE_D = 2 // diagonal dup-corner connector
};

#endif // DIA_CFG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
