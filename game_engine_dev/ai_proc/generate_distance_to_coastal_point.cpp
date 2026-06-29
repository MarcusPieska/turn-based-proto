//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_to_coastal_point.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const u16 k_dep_none = 0xFFFFu;
static const i16 k_dx4[] = {0, 1, 0, -1};
static const i16 k_dy4[] = {-1, 0, 1, 0};

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static bool is_walk (u8 t) {
    return t != TERR_MOUNTAINS[0] && !is_water(t);
}

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static u8 terr_at (const u8* terrain, u16 w, u16 x, u16 y) {
    return terrain[tidx(w, x, y)];
}

static void flood_coast_ring (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 tile_n,
    const u16* coast_dep,
    u16 sx,
    u16 sy,
    u16* ring,
    u16* out_max) {
    *out_max = 0;
    for (u32 i = 0; i < tile_n; ++i) {
        ring[i] = k_dep_none;
    }
    const u32 si = tidx(w, sx, sy);
    if (!is_walk(terr_at(terrain, w, sx, sy))) {
        return;
    }
    if (coast_dep[si] == k_dep_none || coast_dep[si] > Generate_DistanceToCoastalPoint::k_coast_band) {
        return;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return;
    }
    ring[si] = 0;
    u32 qn = 0;
    q[qn++] = si;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 cur = ring[i];
        if (cur >= 65534u) {
            continue;
        }
        if (cur > *out_max) {
            *out_max = cur;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_walk(terr_at(terrain, w, cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (coast_dep[ni] == k_dep_none || coast_dep[ni] > Generate_DistanceToCoastalPoint::k_coast_band) {
                continue;
            }
            if (ring[ni] != k_dep_none) {
                continue;
            }
            ring[ni] = nxt;
            q[qn++] = ni;
            if (nxt > *out_max) {
                *out_max = nxt;
            }
        }
    }
    delete[] q;
}

//================================================================================================================================
//=> - Generate_DistanceToCoastalPoint -
//================================================================================================================================

u16* Generate_DistanceToCoastalPoint::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const u16* coast_dist,
    u16 px,
    u16 py,
    u16* out_max) {
    if (terrain == nullptr || coast_dist == nullptr || w == 0 || h == 0 || out_max == nullptr) {
        return nullptr;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* ring = new u16[tile_n];
    if (ring == nullptr) {
        return nullptr;
    }
    flood_coast_ring(terrain, w, h, tile_n, coast_dist, px, py, ring, out_max);
    return ring;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
