//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef GAME_MAP_DEFS_H
#define GAME_MAP_DEFS_H

#include "game_primitives.h"

//================================================================================================================================
//=> - Climate classes -
//================================================================================================================================

#define CLIMATE_NONE 0
#define CLIMATE_GRASSLAND 1
#define CLIMATE_PLAINS 2
#define CLIMATE_DESERT 3
#define CLIMATE_BLACK_SOIL 4
#define CLIMATE_WT_MAX 99

//================================================================================================================================
//=> - Overlay -
//================================================================================================================================

#define OVERLAY_NONE 0

static const u8 OV_VIZ_WATER_R = 180u;
static const u8 OV_VIZ_WATER_G = 180u;
static const u8 OV_VIZ_WATER_B = 180u;
static const u8 OV_VIZ_LAND_R = 72u;
static const u8 OV_VIZ_LAND_G = 72u;
static const u8 OV_VIZ_LAND_B = 72u;

//================================================================================================================================
//=> - Terrain rows: [0]=class id, [1..3]=RGB at save -
//================================================================================================================================

static const u8 TERR_NONE[4] = {0, 255, 0, 255};
static const u8 TERR_OCEAN[4] = {1, 14, 52, 112};
static const u8 TERR_SEA[4] = {2, 38, 102, 188};
static const u8 TERR_COASTAL[4] = {3, 118, 182, 242};
static const u8 TERR_PLAINS[4] = {4, 34, 112, 48};
static const u8 TERR_HILLS[4] = {5, 50, 140, 78};
static const u8 TERR_MOUNTAINS[4] = {6, 76, 48, 30};

static const u8 TERR_INLAND_SEA[4] = {7, 39, 103, 189};
static const u8 TERR_INLAND_LAKE[4] = {8, 119, 183, 243};

static const u8 TERR_TILE_SENTINEL[4] = {255, 50, 50, 50};

//================================================================================================================================
//=> - Overlay rows: [0]=class id, [1..3]=RGB at save -
//================================================================================================================================

static const u8 OV_NONE[4] = {0, 72, 72, 72};
static const u8 OV_FOREST[4] = {1, 34, 139, 34};
static const u8 OV_SWAMP[4] = {2, 170, 210, 170};
static const u8 OV_JUNGLE[4] = {3, 0, 90, 40};
static const u8 OV_GLACIER[4] = {4, 230, 242, 255};

//================================================================================================================================
//=> - Path costs (TM movement) -
//================================================================================================================================

static const u16 PATH_MP_TURN = 1000u;
static const u16 PATH_COST_STEP = 1000u;
static const u16 PATH_COST_HILL = 2000u;
static const u16 PATH_COST_RIV = 250u;

//================================================================================================================================
//=> - Climate RGB -
//================================================================================================================================

static inline void climate_to_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (cls == CLIMATE_GRASSLAND) {
        *r = 90;
        *g = 170;
        *b = 50;
        return;
    }
    if (cls == CLIMATE_PLAINS) {
        *r = 210;
        *g = 200;
        *b = 80;
        return;
    }
    if (cls == CLIMATE_DESERT) {
        *r = 210;
        *g = 160;
        *b = 70;
        return;
    }
    if (cls == CLIMATE_BLACK_SOIL) {
        *r = 48;
        *g = 128;
        *b = 38;
        return;
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static inline u8 climate_from_rgb (u8 r, u8 g, u8 b, bool* matched) {
    if (matched != nullptr) {
        *matched = false;
    }
    if (r == 90 && g == 170 && b == 50) {
        if (matched != nullptr) {
            *matched = true;
        }
        return CLIMATE_GRASSLAND;
    }
    if (r == 210 && g == 200 && b == 80) {
        if (matched != nullptr) {
            *matched = true;
        }
        return CLIMATE_PLAINS;
    }
    if (r == 210 && g == 160 && b == 70) {
        if (matched != nullptr) {
            *matched = true;
        }
        return CLIMATE_DESERT;
    }
    if (r == 48 && g == 128 && b == 38) {
        if (matched != nullptr) {
            *matched = true;
        }
        return CLIMATE_BLACK_SOIL;
    }
    if (r == 0 && g == 0 && b == 0) {
        if (matched != nullptr) {
            *matched = true;
        }
        return CLIMATE_NONE;
    }
    return CLIMATE_NONE;
}

//================================================================================================================================
//=> - Overlay RGB -
//================================================================================================================================

static inline bool overlay_is_water_terr (u8 terr_cls) {
    return terr_cls == TERR_OCEAN[0] || terr_cls == TERR_SEA[0] || terr_cls == TERR_COASTAL[0]
        || terr_cls == TERR_INLAND_SEA[0] || terr_cls == TERR_INLAND_LAKE[0];
}

static inline void overlay_rgb_from_class (u8 ov_cls, u8* r, u8* g, u8* b) {
    static const u8* const k_ov_rows[] = {
        OV_NONE,
        OV_FOREST,
        OV_SWAMP,
        OV_JUNGLE,
        OV_GLACIER};
    for (unsigned i = 0; i < sizeof(k_ov_rows) / sizeof(k_ov_rows[0]); ++i) {
        const u8* row = k_ov_rows[i];
        if (row[0] == ov_cls) {
            *r = row[1];
            *g = row[2];
            *b = row[3];
            return;
        }
    }
    *r = OV_NONE[1];
    *g = OV_NONE[2];
    *b = OV_NONE[3];
}

static inline u8 overlay_class_from_rgb (u8 r, u8 g, u8 b, bool* matched) {
    if (matched != nullptr) {
        *matched = false;
    }
    static const u8* const k_ov_rows[] = {
        OV_NONE,
        OV_FOREST,
        OV_SWAMP,
        OV_JUNGLE,
        OV_GLACIER};
    for (unsigned i = 0; i < sizeof(k_ov_rows) / sizeof(k_ov_rows[0]); ++i) {
        const u8* row = k_ov_rows[i];
        if (row[1] == r && row[2] == g && row[3] == b) {
            if (matched != nullptr) {
                *matched = true;
            }
            return row[0];
        }
    }
    if (r == OV_VIZ_WATER_R && g == OV_VIZ_WATER_G && b == OV_VIZ_WATER_B) {
        if (matched != nullptr) {
            *matched = true;
        }
        return OV_NONE[0];
    }
    if (r == OV_VIZ_LAND_R && g == OV_VIZ_LAND_G && b == OV_VIZ_LAND_B) {
        if (matched != nullptr) {
            *matched = true;
        }
        return OV_NONE[0];
    }
    return OV_NONE[0];
}

static inline void overlay_viz_to_rgb (u8 terr_cls, u8 ov_cls, u8* r, u8* g, u8* b) {
    if (ov_cls != OV_NONE[0] && !overlay_is_water_terr(terr_cls)) {
        overlay_rgb_from_class(ov_cls, r, g, b);
        return;
    }
    if (overlay_is_water_terr(terr_cls)) {
        *r = OV_VIZ_WATER_R;
        *g = OV_VIZ_WATER_G;
        *b = OV_VIZ_WATER_B;
        return;
    }
    *r = OV_VIZ_LAND_R;
    *g = OV_VIZ_LAND_G;
    *b = OV_VIZ_LAND_B;
}

#endif // GAME_MAP_DEFS_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
