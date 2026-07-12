//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_pts.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "wb_que_xy.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_mountain (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool tile_blocked (u8 cls) {
    return is_water(cls) || is_mountain(cls);
}

static bool build_river_pts (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    P1_Gen_RiverPtsRslt* out) {
    if (terrain == nullptr || out == nullptr || w == 0 || h == 0) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "build_river_pts null args");
    }
    out->m_que.clear();
    const i32 jlim = static_cast<i32>(P1_RIVER_LATTICE_STEP) / 2 - 1;
    const u32 jspan = static_cast<u32>(jlim * 2 + 1);
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u16 ly = 0; ly < h; ly = static_cast<u16>(ly + P1_RIVER_LATTICE_STEP)) {
        for (u16 lx = 0; lx < w; lx = static_cast<u16>(lx + P1_RIVER_LATTICE_STEP)) {
            const i32 ox = static_cast<i32>(rng_next(&rng) % jspan) - jlim;
            const i32 oy = static_cast<i32>(rng_next(&rng) % jspan) - jlim;
            const i32 x = static_cast<i32>(lx) + ox;
            const i32 y = static_cast<i32>(ly) + oy;
            if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (tile_blocked(terrain[i])) {
                continue;
            }
            if (!out->m_que.push(static_cast<u16>(x), static_cast<u16>(y))) {
                P1_STEP_ABORT("P1_Gen_RiverPts", "build_river_pts queue full");
            }
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_n = out->m_que.count();
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverPts -
//================================================================================================================================

P1_Gen_RiverPts::P1_Gen_RiverPts (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_n = 0u;
}

P1_Gen_RiverPts::~P1_Gen_RiverPts () {
    clear_rslt();
}

void P1_Gen_RiverPts::clear_rslt () {
    m_rslt.m_que.clear();
    m_rslt.m_n = 0u;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverPts::generate (const u8* terrain, u16 w, u16 h) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "null terrain or invalid map size");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "size mismatch");
    }
    if (!build_river_pts(terrain, w, h, m_prm.m_seed, &m_rslt)) {
        return false;
    }
    if (m_rslt.m_n == 0u || !m_rslt.m_que.ok()) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "no valid lattice points");
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_RiverPts::generate () {
    P1_STEP_ABORT("P1_Gen_RiverPts", "generate() requires terrain input");
}

bool P1_Gen_RiverPts::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverPtsRslt& P1_Gen_RiverPts::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
