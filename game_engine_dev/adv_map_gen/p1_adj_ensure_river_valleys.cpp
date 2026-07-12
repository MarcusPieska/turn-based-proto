//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_ensure_river_valleys.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Valley distance tuning -
//================================================================================================================================

#define P1_RIV_VAL_DIST_HILL_RIVER 1u
#define P1_RIV_VAL_DIST_NARROW_VALLEY 3u
#define P1_RIV_VAL_DIST_MID_VALLEY 10u
#define P1_RIV_VAL_DIST_BROAD_VALLEY 14u
#define P1_RIV_VAL_DIST_OPEN_VALLEY 18u
#define P1_RIV_VAL_MID_PLAIN_PCT 50u

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
static const i32 k_dx20[20] = {-1, 0, 1, -2, -1, 0, 1, 2, -2, -1, 1, 2, -2, -1, 0, 1, 2, -1, 0, 1};
static const i32 k_dy20[20] = {-2, -2, -2, -1, -1, -1, -1, -1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2};
static const u8 k_adj20_outer[20] = {1, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 1, 1, 1};

struct Rng32 {
    u32 m_s;
};

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_elev_land (u8 cls) {
    return cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool tile_adj4_water (const u8* terrain, u16 w, u16 h, u16 x, u16 y) {
    for (i32 k = 0; k < 4; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (is_water(terrain[ti])) {
            return true;
        }
    }
    return false;
}

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static u32 tile_rng (u32 seed, u16 x, u16 y) {
    Rng32 g;
    rng_seed(&g, seed ^ (static_cast<u32>(x) * 65537u + static_cast<u32>(y)));
    return rng_next(&g);
}

static void set_plain_if_elev (u8* terrain, u32 i) {
    if (is_elev_land(terrain[i])) {
        terrain[i] = TERR_PLAINS[0];
    }
}

static void adj_riv_tile (u8* terrain, u16 w, u16 h, u16 x, u16 y, u32 i, u16 rdd) {
    if (!is_land(terrain[i])) {
        return;
    }
    if (rdd <= P1_RIV_VAL_DIST_HILL_RIVER) {
        if (terrain[i] == TERR_MOUNTAINS[0] && !tile_adj4_water(terrain, w, h, x, y)) {
            terrain[i] = TERR_HILLS[0];
        }
    } else if (rdd <= P1_RIV_VAL_DIST_NARROW_VALLEY) {
        set_plain_if_elev(terrain, i);
    } else if (rdd <= P1_RIV_VAL_DIST_MID_VALLEY) {
        set_plain_if_elev(terrain, i);
    } else {
        set_plain_if_elev(terrain, i);
    }
}

static void adj_adj8_from_riv (
    u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u32 seed,
    bool rng_mid) 
{
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx8[k];
        const i32 ny = static_cast<i32>(y) + k_dy8[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (!is_land(terrain[ti]) || !is_elev_land(terrain[ti])) {
            continue;
        }
        if (rng_mid) {
            if (tile_rng(seed, static_cast<u16>(nx), static_cast<u16>(ny)) % 100u
                >= static_cast<u32>(P1_RIV_VAL_MID_PLAIN_PCT)) {
                continue;
            }
        }
        terrain[ti] = TERR_PLAINS[0];
    }
}

static void adj_adj20_from_riv (
    u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u32 seed,
    bool rng_outer) 
{
    for (i32 k = 0; k < 20; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx20[k];
        const i32 ny = static_cast<i32>(y) + k_dy20[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ti = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (!is_land(terrain[ti]) || !is_elev_land(terrain[ti])) {
            continue;
        }
        if (rng_outer && k_adj20_outer[k] != 0
            && tile_rng(seed, static_cast<u16>(nx), static_cast<u16>(ny)) % 100u
                >= static_cast<u32>(P1_RIV_VAL_MID_PLAIN_PCT)) {
            continue;
        }
        terrain[ti] = TERR_PLAINS[0];
    }
}

static void adj_stamp_from_riv (
    u8* terrain,
    u16 w,
    u16 h,
    u16 x,
    u16 y,
    u16 rdd,
    u32 seed) 
{
    if (rdd <= P1_RIV_VAL_DIST_NARROW_VALLEY) {
        return;
    }
    if (rdd <= P1_RIV_VAL_DIST_MID_VALLEY) {
        adj_adj8_from_riv(terrain, w, h, x, y, seed, true);
        return;
    }
    if (rdd <= P1_RIV_VAL_DIST_BROAD_VALLEY) {
        adj_adj20_from_riv(terrain, w, h, x, y, seed, true);
        return;
    }
    if (rdd <= P1_RIV_VAL_DIST_OPEN_VALLEY) {
        adj_adj20_from_riv(terrain, w, h, x, y, seed, false);
        return;
    }
    adj_adj20_from_riv(terrain, w, h, x, y, seed, false);
}

static bool apply_ensure_river_valleys (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* riv,
    const u16* dist_dn,
    u32 seed) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (n == 0 || terrain == nullptr || riv == nullptr || dist_dn == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (riv[i] == 0) {
                continue;
            }
            const u16 rdd = dist_dn[i];
            if (rdd == 0) {
                continue;
            }
            adj_riv_tile(terrain, w, h, x, y, i, rdd);
            adj_stamp_from_riv(terrain, w, h, x, y, rdd, seed);
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Adj_EnsureRiverValleys -
//================================================================================================================================

P1_Adj_EnsureRiverValleys::P1_Adj_EnsureRiverValleys (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_adjust(false) {
}

bool P1_Adj_EnsureRiverValleys::adjust (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* riv,
    const u16* dist_dn) 
{
    m_valid_adjust = false;
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || riv == nullptr || dist_dn == nullptr) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!apply_ensure_river_valleys(terrain, w, h, riv, dist_dn, m_prm.m_seed)) {
        return false;
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_EnsureRiverValleys::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
