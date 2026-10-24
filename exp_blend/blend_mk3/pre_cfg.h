//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef PRE_CFG_H
#define PRE_CFG_H

#include <cstdint>

//================================================================================================================================
//=> - Grid and corner catalog -
//================================================================================================================================

#define TILE_PX 100
#define CORNER_UNIT 6
#define TERRAIN_N 4
#define TERRAIN_ID_MAX 16
#define CORNER_MAG_N 4
#define CORNER_AXIS_N 8
#define CORNER_CAT_N (CORNER_AXIS_N * CORNER_AXIS_N)
#define CORNER_PULL_N 2
#define SOLO_DUP_PCT 30

static const int CORNER_MAG[CORNER_MAG_N] = { 0, 10, 20, 30 };
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
    EDGE_H = 0, // horizontal shared border
    EDGE_V = 1, // vertical shared border
    EDGE_D = 2 // diagonal dup-corner connector
};

inline int corner_off (int axis_i) {
    int mag = CORNER_MAG[axis_i % CORNER_MAG_N];
    return (axis_i < CORNER_MAG_N) ? -mag : mag;
}

inline int corner_axis_i (int off) {
    if (off <= 0) {
        for (int i = 0; i < CORNER_MAG_N; i++) if (CORNER_MAG[i] == -off) return i;
        return 0;
    }
    for (int i = 0; i < CORNER_MAG_N; i++) if (CORNER_MAG[i] == off) return CORNER_MAG_N + i;
    return CORNER_MAG_N;
}

#endif // PRE_CFG_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
