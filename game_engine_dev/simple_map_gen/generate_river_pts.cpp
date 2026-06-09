//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_pts.h"

#include "generator_constants.h"

#include <cstdlib>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mountain (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool tile_blocked (u8 cls) {
    return is_water(cls) || is_mountain(cls);
}

//================================================================================================================================
//=> - Generate_RiverPts -
//================================================================================================================================

RiverPtsResult* Generate_RiverPts::generate (const u8* terrain, u16 w, u16 h, u32 seed) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 cap = ((static_cast<u32>(w) / RIVER_LATTICE_STEP) + 1u)
        * ((static_cast<u32>(h) / RIVER_LATTICE_STEP) + 1u);
    RiverPtsResult* out = new RiverPtsResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->pts = new RiverPt[cap];
    if (out->pts == nullptr) {
        delete out;
        return nullptr;
    }
    out->n = 0;
    std::srand(seed);
    for (u16 ly = 0; ly < h; ly = static_cast<u16>(ly + RIVER_LATTICE_STEP)) {
        for (u16 lx = 0; lx < w; lx = static_cast<u16>(lx + RIVER_LATTICE_STEP)) {
            const i32 ox = static_cast<i32>((std::rand() % 11) - 5);
            const i32 oy = static_cast<i32>((std::rand() % 11) - 5);
            const i32 x = static_cast<i32>(lx) + ox;
            const i32 y = static_cast<i32>(ly) + oy;
            if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (tile_blocked(terrain[i])) {
                continue;
            }
            out->pts[out->n].x = static_cast<u16>(x);
            out->pts[out->n].y = static_cast<u16>(y);
            out->n = out->n + 1u;
        }
    }
    return out;
}

void Generate_RiverPts::free_result (RiverPtsResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->pts;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
