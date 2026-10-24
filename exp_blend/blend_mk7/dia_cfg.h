//================================================================================================================================
// Diamond tiles with 10x10 subtilles (mk6): same pattern banks as mk5, isometric draw.
//================================================================================================================================

#ifndef DIA_CFG_H
#define DIA_CFG_H

#include <cstdint>

//================================================================================================================================
//=> - Grid -
//================================================================================================================================

#define SUBTILE_DIV 10
#define TILE_SPACE_X 100
#define TILE_H 50
#define TILE_HALF_W (TILE_SPACE_X / 2)
#define TILE_HALF_H (TILE_H / 2)
#define SUB_HALF_W (TILE_HALF_W / SUBTILE_DIV)
#define SUB_HALF_H (TILE_HALF_H / SUBTILE_DIV)
#define TERRAIN_N 9
#define TERRAIN_ID_MAX 16

//================================================================================================================================
//=> - Terrain ids and flat colors -
//================================================================================================================================

enum TerrainId {
    TER_DESERT = 0, // pale yellow
    TER_PLAINS = 1, // wheat yellow
    TER_GRASSLANDS = 2, // green
    TER_BLACKSOIL = 3, // deeper darker green
    TER_OCEAN = 4, // deep blue (beach water)
    TER_SEA = 5, // mid blue (beach water)
    TER_COAST = 6, // shallow blue (beach water)
    TER_INLAND_SEA = 7, // inland; no beach swap
    TER_INLAND_LAKE = 8 // inland; no beach swap
};

static inline bool ter_beach_water (int id) {
    return id == TER_OCEAN || id == TER_SEA || id == TER_COAST;
}

static inline bool ter_any_water (int id) {
    return ter_beach_water (id) || id == TER_INLAND_SEA || id == TER_INLAND_LAKE;
}

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
    Rgb (35, 85, 48),
    Rgb (20, 50, 120),
    Rgb (40, 90, 160),
    Rgb (70, 140, 180),
    Rgb (35, 80, 150),
    Rgb (55, 120, 170)
};

struct Vx {
    int32_t x;
    int32_t y;

    Vx () : x(0), y(0) {}
    Vx (int32_t xx, int32_t yy) : x(xx), y(yy) {}
};

#endif // DIA_CFG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
