//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_pts.h"
#include "generator_constants.h"
#include "p1_step_log.h"
#include "wb_que_xy.h"

#include <cstring>

struct Rng32 {
    u32 m_s;
};

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

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

bool p1_step_pt_lay_init (P1_StepPtLay* lay, u16 w, u16 h, u16 step) {
    if (lay == nullptr || w == 0 || h == 0 || step < 2u) {
        return false;
    }
    const u32 cap = p1_river_pts_max(w, h, step);
    if (cap == 0u) {
        return false;
    }
    u8* mrk = new u8[cap];
    u16* ax = new u16[cap];
    u16* ay = new u16[cap];
    if (mrk == nullptr || ax == nullptr || ay == nullptr) {
        delete[] mrk;
        delete[] ax;
        delete[] ay;
        return false;
    }
    std::memset(mrk, 0, cap);
    lay->m_step = step;
    lay->m_w = w;
    lay->m_cap = cap;
    lay->m_mrk = mrk;
    lay->m_ax = ax;
    lay->m_ay = ay;
    return true;
}

void p1_step_pt_lay_free (P1_StepPtLay* lay) {
    if (lay == nullptr) {
        return;
    }
    delete[] lay->m_mrk;
    delete[] lay->m_ax;
    delete[] lay->m_ay;
    lay->m_mrk = nullptr;
    lay->m_ax = nullptr;
    lay->m_ay = nullptr;
    lay->m_cap = 0u;
    lay->m_step = 0u;
    lay->m_w = 0u;
}

bool p1_step_pt_lay_at (const P1_StepPtLay* lay, u16 alx, u16 aly, u16* ox, u16* oy) {
    if (lay == nullptr || lay->m_mrk == nullptr || lay->m_ax == nullptr || lay->m_ay == nullptr || ox == nullptr || oy == nullptr) {
        return false;
    }
    const u32 ai = p1_step_pt_anchor_i(alx, aly, lay->m_step, lay->m_w);
    if (ai >= lay->m_cap || lay->m_mrk[ai] == 0u) {
        return false;
    }
    *ox = lay->m_ax[ai];
    *oy = lay->m_ay[ai];
    return true;
}

bool p1_stamp_land_step_lay (const u8* terrain, u16 w, u16 h, u32 seed, P1_StepPtLay* lay) {
    if (terrain == nullptr || lay == nullptr || w == 0 || h == 0 || lay->m_step < 2u || lay->m_mrk == nullptr) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "stamp_land_step_lay null args");
    }
    const u16 step = lay->m_step;
    const i32 jlim = (static_cast<i32>(step) / 2 - 1);
    const u32 jspan = static_cast<u32>(jlim * 2 + 1);
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u16 ly = 0; ly < h; ly = static_cast<u16>(ly + step)) {
        for (u16 lx = 0; lx < w; lx = static_cast<u16>(lx + step)) {
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
            const u32 ai = p1_step_pt_anchor_i(lx, ly, step, w);
            if (ai >= lay->m_cap) {
                P1_STEP_ABORT("P1_Gen_RiverPts", "stamp_land_step_lay anchor overflow");
            }
            lay->m_mrk[ai] = 1u;
            lay->m_ax[ai] = static_cast<u16>(x);
            lay->m_ay[ai] = static_cast<u16>(y);
        }
    }
    return true;
}

bool p1_push_ocean_pts (
    const u16* ocn,
    u16 w,
    u16 h,
    u16 ocean_n,
    u16 largest_idx,
    WB_QueXY* que,
    u16* ocn_sec_n) {
    if (ocn == nullptr || que == nullptr || ocn_sec_n == nullptr || ocean_n == 0u) {
        return false;
    }
    *ocn_sec_n = 0u;
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 n = wi * hi;
    u32 sx[P1_OCEAN_IDX_MAX + 1u];
    u32 sy[P1_OCEAN_IDX_MAX + 1u];
    u32 sc[P1_OCEAN_IDX_MAX + 1u];
    for (u32 i = 0; i <= static_cast<u32>(P1_OCEAN_IDX_MAX); ++i) {
        sx[i] = 0u;
        sy[i] = 0u;
        sc[i] = 0u;
    }
    for (u32 py = 0; py < hi; ++py) {
        for (u32 px = 0; px < wi; ++px) {
            const u32 ti = py * wi + px;
            const u16 idx = ocn[ti];
            if (idx == static_cast<u16>(P1_OCEAN_IDX_NONE) || idx > ocean_n) {
                continue;
            }
            sc[static_cast<u32>(idx)]++;
            sx[static_cast<u32>(idx)] += px;
            sy[static_cast<u32>(idx)] += py;
        }
    }
    for (u16 idx = 1u; idx <= ocean_n; ++idx) {
        if (idx == largest_idx || sc[static_cast<u32>(idx)] == 0u) {
            continue;
        }
        const u32 cnt = sc[static_cast<u32>(idx)];
        u16 cx = static_cast<u16>(sx[static_cast<u32>(idx)] / cnt);
        u16 cy = static_cast<u16>(sy[static_cast<u32>(idx)] / cnt);
        if (ocn[static_cast<u32>(cy) * wi + static_cast<u32>(cx)] != idx) {
            bool found = false;
            for (u32 ti = 0; ti < n; ++ti) {
                if (ocn[ti] != idx) {
                    continue;
                }
                cx = static_cast<u16>(ti % wi);
                cy = static_cast<u16>(ti / wi);
                found = true;
                break;
            }
            if (!found) {
                continue;
            }
        }
        if (!que->push(cx, cy)) {
            return false;
        }
        (*ocn_sec_n)++;
    }
    return true;
}

static bool build_land_pts (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    u16 step,
    WB_QueXY* que) {
    if (terrain == nullptr || que == nullptr || w == 0 || h == 0 || step < 2u) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "build_land_pts null args");
    }
    const i32 jlim = (static_cast<i32>(step) / 2 - 1);
    const u32 jspan = static_cast<u32>(jlim * 2 + 1);
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u16 ly = 0; ly < h; ly = static_cast<u16>(ly + step)) {
        for (u16 lx = 0; lx < w; lx = static_cast<u16>(lx + step)) {
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
            if (!que->push(static_cast<u16>(x), static_cast<u16>(y))) {
                P1_STEP_ABORT("P1_Gen_RiverPts", "build_land_pts queue full");
            }
        }
    }
    return true;
}

static bool build_river_pts (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    u16 step,
    const P1_OceanIndexRef& ocean,
    P1_Gen_RiverPtsRslt* out) {
    if (terrain == nullptr || out == nullptr || w == 0 || h == 0) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "build_river_pts null args");
    }
    out->m_que.clear();
    out->m_ocn_sec_n = 0u;
    if (p1_ocean_ref_ok(ocean) && ocean.m_w == w && ocean.m_h == h && ocean.m_ocean_n > 0u) {
        if (!p1_push_ocean_pts(ocean.m_ov, w, h, ocean.m_ocean_n, ocean.m_largest_idx, &out->m_que, &out->m_ocn_sec_n)) {
            P1_STEP_ABORT("P1_Gen_RiverPts", "build_river_pts ocean pts failed");
        }
    }
    if (!build_land_pts(terrain, w, h, seed, step, &out->m_que)) {
        return false;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_n = out->m_que.count();
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverPts -
//================================================================================================================================

P1_Gen_RiverPts::P1_Gen_RiverPts (const P1_RunPrm& prm, u16 lattice_step) :
    m_prm(prm),
    m_lattice_step(lattice_step),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_n = 0u;
}

P1_Gen_RiverPts::~P1_Gen_RiverPts () {
    clear_rslt();
}

void P1_Gen_RiverPts::clear_rslt () {
    m_rslt.m_que.clear();
    m_rslt.m_n = 0u;
    m_rslt.m_ocn_sec_n = 0;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_RiverPts::generate (const u8* terrain, u16 w, u16 h) {
    P1_OceanIndexRef ocn_ref = {};
    return generate(terrain, w, h, ocn_ref);
}

bool P1_Gen_RiverPts::generate (const u8* terrain, u16 w, u16 h, const P1_OceanIndexRef& ocean) {
    m_valid_generation = false;
    clear_rslt();
    if (terrain == nullptr || !p1_map_size_ok(w, h)) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "null terrain or invalid map size");
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "size mismatch");
    }
    if (m_lattice_step < 2u) {
        P1_STEP_ABORT("P1_Gen_RiverPts", "invalid lattice step");
    }
    if (!build_river_pts(terrain, w, h, m_prm.m_seed, m_lattice_step, ocean, &m_rslt)) {
        return false;
    }
    if (m_rslt.m_n == 0u || m_rslt.m_n <= static_cast<u32>(m_rslt.m_ocn_sec_n) || !m_rslt.m_que.ok()) {
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

P1_Gen_RiverPtsRslt& P1_Gen_RiverPts::rslt_mut () {
    return m_rslt;
}

u16 P1_Gen_RiverPts::lattice_step () const {
    return m_lattice_step;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
