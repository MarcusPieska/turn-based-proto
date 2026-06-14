//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_gen_river_pts.h"
#include "generator_constants.h"

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

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    std::fwrite(rgb, 1, nbytes, fp);
    std::fclose(fp);
    return true;
}

static void set_px_rgb (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    if (px >= w || py >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static bool build_river_pts (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    P1_Gen_RiverPtsRslt* out) {
    if (terrain == nullptr || out == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 cap = ((static_cast<u32>(w) / P1_RIVER_LATTICE_STEP) + 1u)
        * ((static_cast<u32>(h) / P1_RIVER_LATTICE_STEP) + 1u);
    P1_RiverPt* pts = new P1_RiverPt[cap];
    if (pts == nullptr) {
        return false;
    }
    const i32 jlim = static_cast<i32>(P1_RIVER_LATTICE_STEP) / 2 - 1;
    const u32 jspan = static_cast<u32>(jlim * 2 + 1);
    Rng32 rng;
    rng_seed(&rng, seed);
    u32 n = 0;
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
            pts[n].m_id = static_cast<u16>(n);
            pts[n].m_x = static_cast<u16>(x);
            pts[n].m_y = static_cast<u16>(y);
            n++;
        }
    }
    out->m_w = w;
    out->m_h = h;
    out->m_n = n;
    out->m_pts = pts;
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
    m_rslt.m_n = 0;
    m_rslt.m_pts = nullptr;
}

P1_Gen_RiverPts::~P1_Gen_RiverPts () {
    clear_rslt();
}

void P1_Gen_RiverPts::clear_rslt () {
    delete[] m_rslt.m_pts;
    m_rslt.m_pts = nullptr;
    m_rslt.m_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverPts::generate (const u8* terrain, u16 w, u16 h) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_river_pts(terrain, w, h, m_prm.m_seed, &m_rslt)) {
        return false;
    }
    m_valid_generation = m_rslt.m_n > 0;
    return m_valid_generation;
}

bool P1_Gen_RiverPts::generate () {
    return false;
}

bool P1_Gen_RiverPts::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_RiverPtsRslt& P1_Gen_RiverPts::result () const {
    return m_rslt;
}

void P1_Gen_RiverPts::save_output (cstr path, const u8* terrain) const {
    if (!m_valid_generation || path == nullptr || terrain == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(npx) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < npx; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        const u8 cls = terrain[i];
        if (cls == TERR_OCEAN[0]) {
            r = TERR_OCEAN[1]; g = TERR_OCEAN[2]; b = TERR_OCEAN[3];
        } else if (cls == TERR_SEA[0]) {
            r = TERR_SEA[1]; g = TERR_SEA[2]; b = TERR_SEA[3];
        } else if (cls == TERR_COASTAL[0]) {
            r = TERR_COASTAL[1]; g = TERR_COASTAL[2]; b = TERR_COASTAL[3];
        } else if (cls == TERR_PLAINS[0]) {
            r = TERR_PLAINS[1]; g = TERR_PLAINS[2]; b = TERR_PLAINS[3];
        } else if (cls == TERR_HILLS[0]) {
            r = TERR_HILLS[1]; g = TERR_HILLS[2]; b = TERR_HILLS[3];
        } else if (cls == TERR_MOUNTAINS[0]) {
            r = TERR_MOUNTAINS[1]; g = TERR_MOUNTAINS[2]; b = TERR_MOUNTAINS[3];
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 p = 0; p < m_rslt.m_n; ++p) {
        const u16 px = m_rslt.m_pts[p].m_x;
        const u16 py = m_rslt.m_pts[p].m_y;
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dx = -2; dx <= 2; ++dx) {
                if (dx * dx + dy * dy > 4) {
                    continue;
                }
                const i32 x = static_cast<i32>(px) + dx;
                const i32 y = static_cast<i32>(py) + dy;
                if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
                    continue;
                }
                set_px_rgb(rgb, w, h, static_cast<u16>(x), static_cast<u16>(y), 255, 0, 0);
            }
        }
    }
    save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
