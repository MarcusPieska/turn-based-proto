//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_distance_to_ocean_coast.h"
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

static bool is_ocean (u8 t) {
    return t == TERR_OCEAN[0];
}

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

static void flood_ocean_wtr (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 tile_n,
    u16* oc) {
    for (u32 i = 0; i < tile_n; ++i) {
        oc[i] = 0;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_ocean(terr_at(terrain, w, x, y))) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            oc[i] = 1;
            q[qn++] = i;
        }
    }
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (u32 k = 0; k < 4u; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0) {
                continue;
            }
            const u16 cx = static_cast<u16>(nx);
            const u16 cy = static_cast<u16>(ny);
            if (cx >= w || cy >= h || !is_water(terr_at(terrain, w, cx, cy))) {
                continue;
            }
            const u32 ni = tidx(w, cx, cy);
            if (oc[ni] != 0) {
                continue;
            }
            oc[ni] = 1;
            q[qn++] = ni;
        }
    }
    delete[] q;
}

static bool has_ocn_wtr_nbr (
    const u8* terrain,
    const u16* oc,
    u16 x,
    u16 y,
    u16 w,
    u16 h) {
    for (u32 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + k_dx4[k];
        const i32 ny = static_cast<i32>(y) + k_dy4[k];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 ax = static_cast<u16>(nx);
        const u16 ay = static_cast<u16>(ny);
        if (ax >= w || ay >= h) {
            continue;
        }
        const u32 i = tidx(w, ax, ay);
        if (is_water(terr_at(terrain, w, ax, ay)) && oc[i] != 0) {
            return true;
        }
    }
    return false;
}

static void flood_coast_dep (
    const u8* terrain,
    u16 w,
    u16 h,
    u32 tile_n,
    const u16* oc,
    u16* dep,
    u16* out_max) {
    *out_max = 0;
    for (u32 i = 0; i < tile_n; ++i) {
        dep[i] = k_dep_none;
    }
    u32* q = new u32[tile_n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!is_walk(terr_at(terrain, w, x, y))) {
                continue;
            }
            if (!has_ocn_wtr_nbr(terrain, oc, x, y, w, h)) {
                continue;
            }
            const u32 i = tidx(w, x, y);
            dep[i] = 0;
            q[qn++] = i;
        }
    }
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 cur = dep[i];
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
            if (dep[ni] != k_dep_none) {
                continue;
            }
            dep[ni] = nxt;
            q[qn++] = ni;
            if (nxt > *out_max) {
                *out_max = nxt;
            }
        }
    }
    delete[] q;
}

//================================================================================================================================
//=> - Generate_DistanceToOceanCoast -
//================================================================================================================================

u16* Generate_DistanceToOceanCoast::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    u16* out_max) {
    if (terrain == nullptr || w == 0 || h == 0 || out_max == nullptr) {
        return nullptr;
    }
    const u32 tile_n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dep = new u16[tile_n];
    u16* oc = new u16[tile_n];
    if (dep == nullptr || oc == nullptr) {
        delete[] dep;
        delete[] oc;
        return nullptr;
    }
    flood_ocean_wtr(terrain, w, h, tile_n, oc);
    flood_coast_dep(terrain, w, h, tile_n, oc, dep, out_max);
    delete[] oc;
    return dep;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
