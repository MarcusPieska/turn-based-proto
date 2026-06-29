//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_climate.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

struct P1_ClimateTileVal {
    u32 m_val;
    u16 m_x;
    u16 m_y;
};

static const u16 k_inf = 0xFFFFu;

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_open_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static u8 clamp_wt (u8 w) {
    return (w > CLIMATE_WT_MAX) ? CLIMATE_WT_MAX : w;
}

static u8 scale_dist_u16 (u16 d, u16 max_dist, bool land_all_zero) {
    if (land_all_zero) {
        return 255;
    }
    const u32 dv = static_cast<u32>(d);
    const u32 md = static_cast<u32>(max_dist);
    if (md == 0) {
        return 0;
    }
    return static_cast<u8>((dv * 255u) / md);
}

static f32 dist_to_equator (f32 px, f32 py, u16 w, u16 h) {
    if (w == 0 || h == 0) {
        return 0.f;
    }
    const f32 ax = 0.f;
    const f32 ay = static_cast<f32>(h) - 1.f;
    const f32 bx = static_cast<f32>(w) - 1.f;
    const f32 by = 0.f;
    const f32 abx = bx - ax;
    const f32 aby = by - ay;
    const f32 apx = px - ax;
    const f32 apy = py - ay;
    const f32 cross = abx * apy - aby * apx;
    const f32 ab_len = std::sqrt(abx * abx + aby * aby);
    if (ab_len <= 0.f) {
        return 0.f;
    }
    return std::fabs(cross) / ab_len;
}

static u8 scale_lat (f32 dist, f32 max_dist) {
    if (max_dist <= 0.f) {
        return 0;
    }
    f32 t = dist / max_dist;
    if (t < 0.f) {
        t = 0.f;
    }
    if (t > 1.f) {
        t = 1.f;
    }
    const u32 v = static_cast<u32>(t * 255.f + 0.5f);
    return static_cast<u8>(v > 255u ? 255u : v);
}

static u32 tile_score (
    u8 river,
    u8 open_w,
    u8 lat,
    u8 plain_w,
    const P1_Gen_ClimateOverlayWts* wts) 
{
    const u32 wr = static_cast<u32>(clamp_wt(wts->m_w_dist_river));
    const u32 wo = static_cast<u32>(clamp_wt(wts->m_w_open_dist_water));
    const u32 wl = static_cast<u32>(clamp_wt(wts->m_w_latitude));
    const u32 wp = static_cast<u32>(clamp_wt(wts->m_w_plain_dist_water));
    const u32 sr = static_cast<u32>(255u - static_cast<u32>(river));
    const u32 so = static_cast<u32>(255u - static_cast<u32>(open_w));
    const u32 sl = static_cast<u32>(255u - static_cast<u32>(lat));
    const u32 sp = static_cast<u32>(255u - static_cast<u32>(plain_w));
    return sr * wr + so * wo + sl * wl + sp * wp;
}

static u32 pct_count (u32 n, u8 pct) {
    return (n * static_cast<u32>(pct) + 50u) / 100u;
}

static int cmp_climate_tile (const void* a, const void* b) {
    const P1_ClimateTileVal* ta = static_cast<const P1_ClimateTileVal*>(a);
    const P1_ClimateTileVal* tb = static_cast<const P1_ClimateTileVal*>(b);
    if (ta->m_val != tb->m_val) {
        return (ta->m_val > tb->m_val) ? -1 : 1;
    }
    if (ta->m_y != tb->m_y) {
        return (ta->m_y < tb->m_y) ? -1 : 1;
    }
    if (ta->m_x < tb->m_x) {
        return -1;
    }
    if (ta->m_x > tb->m_x) {
        return 1;
    }
    return 0;
}

static u8 pick_adj_climate (u16 w, u16 h, u16 px, u16 py, const u8* terrain, const u8* climate) {
    u32 cnt_grass = 0;
    u32 cnt_plains = 0;
    u32 cnt_desert = 0;
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + dx;
            const i32 ny = static_cast<i32>(py) + dy;
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w)
                || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (!is_land(terrain[j])) {
                continue;
            }
            const u8 c = climate[j];
            if (c == CLIMATE_GRASSLAND) {
                cnt_grass++;
            } else if (c == CLIMATE_PLAINS) {
                cnt_plains++;
            } else if (c == CLIMATE_DESERT) {
                cnt_desert++;
            }
        }
    }
    u8 pick = CLIMATE_NONE;
    u32 best = 0;
    if (cnt_grass > best) {
        best = cnt_grass;
        pick = CLIMATE_GRASSLAND;
    }
    if (cnt_plains > best) {
        best = cnt_plains;
        pick = CLIMATE_PLAINS;
    }
    if (cnt_desert > best) {
        best = cnt_desert;
        pick = CLIMATE_DESERT;
    }
    return pick;
}

static bool has_adj_climate (u16 w, u16 h, u16 px, u16 py, const u8* terrain, const u8* climate) {
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            if (dx == 0 && dy == 0) {
                continue;
            }
            const i32 nx = static_cast<i32>(px) + dx;
            const i32 ny = static_cast<i32>(py) + dy;
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w)
                || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
            if (!is_land(terrain[j])) {
                continue;
            }
            if (climate[j] != CLIMATE_NONE) {
                return true;
            }
        }
    }
    return false;
}

static void assign_mountain_climate (u16 w, u16 h, const u8* terrain, u8* climate) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    if (q == nullptr) {
        return;
    }
    size_t qh = 0;
    size_t qn = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (terrain[i] != TERR_MOUNTAINS[0] || climate[i] != CLIMATE_NONE) {
                continue;
            }
            if (has_adj_climate(w, h, px, py, terrain, climate)) {
                q[qn++] = i;
            }
        }
    }
    while (qh < qn) {
        const u32 i = q[qh++];
        if (climate[i] != CLIMATE_NONE) {
            continue;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u8 pick = pick_adj_climate(w, h, px, py, terrain, climate);
        if (pick == CLIMATE_NONE) {
            continue;
        }
        climate[i] = pick;
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                if (dx == 0 && dy == 0) {
                    continue;
                }
                const i32 nx = static_cast<i32>(px) + dx;
                const i32 ny = static_cast<i32>(py) + dy;
                if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w)
                    || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 j = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
                if (terrain[j] != TERR_MOUNTAINS[0] || climate[j] != CLIMATE_NONE) {
                    continue;
                }
                q[qn++] = j;
            }
        }
    }
    delete[] q;
}

static bool bfs_land_to_river (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* river,
    u8* out) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    u32* q = new u32[n / 4u + 64u];
    if (dist == nullptr || q == nullptr) {
        delete[] q;
        delete[] dist;
        return false;
    }
    u16 max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
        out[i] = 0;
    }
    size_t qh = 0;
    size_t qn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (river[i] == 0) {
            continue;
        }
        dist[i] = 0;
        q[qn++] = i;
    }
    while (qh < qn) {
        const u32 i = q[qh++];
        const u16 cur = dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        if (cur > max_dist) {
            max_dist = cur;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
    }
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_open_land(terrain[i]) || dist[i] == k_inf) {
            continue;
        }
        land_fin++;
    }
    const bool land_all_zero = (max_dist == 0 && land_fin > 0);
    if (max_dist == 0 && !land_all_zero) {
        max_dist = 1;
    }
    for (u32 i = 0; i < n; ++i) {
        if (!is_open_land(terrain[i]) || dist[i] == k_inf) {
            continue;
        }
        out[i] = scale_dist_u16(dist[i], max_dist, land_all_zero);
    }
    delete[] q;
    delete[] dist;
    return true;
}

static bool bfs_open_land_to_water (u16 w, u16 h, const u8* terrain, u8* out) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    u32* q = new u32[n / 4u + 64u];
    if (dist == nullptr || q == nullptr) {
        delete[] q;
        delete[] dist;
        return false;
    }
    u16 max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
        out[i] = 0;
    }
    size_t qn = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_open_land(terrain[i])) {
                continue;
            }
            bool adj = false;
            if (px > 0 && is_water(terrain[i - 1u])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(px) + 1u < static_cast<u32>(w) && is_water(terrain[i + 1u])) {
                adj = true;
            }
            if (!adj && py > 0 && is_water(terrain[i - static_cast<u32>(w)])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(py) + 1u < static_cast<u32>(h) && is_water(terrain[i + static_cast<u32>(w)])) {
                adj = true;
            }
            if (adj) {
                dist[i] = 0;
                q[qn++] = i;
            }
        }
    }
    size_t qh = 0;
    while (qh < qn) {
        const u32 i = q[qh++];
        const u16 cur = dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        if (cur > max_dist) {
            max_dist = cur;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_open_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
    }
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_open_land(terrain[i]) || dist[i] == k_inf) {
            continue;
        }
        land_fin++;
    }
    const bool land_all_zero = (max_dist == 0 && land_fin > 0);
    if (max_dist == 0 && !land_all_zero) {
        max_dist = 1;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = terrain[i];
        if (is_water(cls) || cls == TERR_MOUNTAINS[0]) {
            continue;
        }
        if (!is_open_land(cls) || dist[i] == k_inf) {
            continue;
        }
        out[i] = scale_dist_u16(dist[i], max_dist, land_all_zero);
    }
    delete[] q;
    delete[] dist;
    return true;
}

static bool bfs_land_to_water (u16 w, u16 h, const u8* terrain, u8* out) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    u32* q = new u32[n / 4u + 64u];
    if (dist == nullptr || q == nullptr) {
        delete[] q;
        delete[] dist;
        return false;
    }
    u16 max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
        out[i] = 0;
    }
    size_t qn = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_land(terrain[i])) {
                continue;
            }
            bool adj = false;
            if (px > 0 && is_water(terrain[i - 1u])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(px) + 1u < static_cast<u32>(w) && is_water(terrain[i + 1u])) {
                adj = true;
            }
            if (!adj && py > 0 && is_water(terrain[i - static_cast<u32>(w)])) {
                adj = true;
            }
            if (!adj && static_cast<u32>(py) + 1u < static_cast<u32>(h) && is_water(terrain[i + static_cast<u32>(w)])) {
                adj = true;
            }
            if (adj) {
                dist[i] = 0;
                q[qn++] = i;
            }
        }
    }
    size_t qh = 0;
    while (qh < qn) {
        const u32 i = q[qh++];
        const u16 cur = dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        if (cur > max_dist) {
            max_dist = cur;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q[qn++] = j;
            }
        }
    }
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terrain[i]) || dist[i] == k_inf) {
            continue;
        }
        land_fin++;
    }
    const bool land_all_zero = (max_dist == 0 && land_fin > 0);
    if (max_dist == 0 && !land_all_zero) {
        max_dist = 1;
    }
    for (u32 i = 0; i < n; ++i) {
        if (is_water(terrain[i]) || !is_land(terrain[i]) || dist[i] == k_inf) {
            continue;
        }
        out[i] = scale_dist_u16(dist[i], max_dist, land_all_zero);
    }
    delete[] q;
    delete[] dist;
    return true;
}

static bool build_latitude (u16 w, u16 h, u8* out) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const f32 d_tl = dist_to_equator(0.5f, 0.5f, w, h);
    const f32 d_br = dist_to_equator(static_cast<f32>(w) - 0.5f, static_cast<f32>(h) - 0.5f, w, h);
    const f32 max_dist = (d_tl > d_br) ? d_tl : d_br;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const f32 cx = static_cast<f32>(px) + 0.5f;
            const f32 cy = static_cast<f32>(py) + 0.5f;
            const f32 d = dist_to_equator(cx, cy, w, h);
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            out[i] = static_cast<u8>(255u - static_cast<u32>(scale_lat(d, max_dist)));
        }
    }
    (void)n;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_Climate -
//================================================================================================================================

P1_Gen_Climate::P1_Gen_Climate (const P1_RunPrm& prm, const P1_Gen_ClimatePrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false) {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_Climate::generate (const u8* terrain, u16 w, u16 h, const u8* river_ov) {
    m_valid_generation = false;
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_ov.clear();
    if (!p1_run_prm_ok(m_prm) || terrain == nullptr || river_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* dist_river = new u8[n];
    u8* open_dist_water = new u8[n];
    u8* latitude = new u8[n];
    u8* plain_dist_water = new u8[n];
    if (dist_river == nullptr || open_dist_water == nullptr || latitude == nullptr || plain_dist_water == nullptr) {
        delete[] plain_dist_water;
        delete[] latitude;
        delete[] open_dist_water;
        delete[] dist_river;
        return false;
    }
    if (!bfs_land_to_river(w, h, terrain, river_ov, dist_river)
        || !bfs_open_land_to_water(w, h, terrain, open_dist_water)
        || !build_latitude(w, h, latitude)
        || !bfs_land_to_water(w, h, terrain, plain_dist_water)) {
        delete[] plain_dist_water;
        delete[] latitude;
        delete[] open_dist_water;
        delete[] dist_river;
        return false;
    }
    u32 tile_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_open_land(terrain[i])) {
            tile_n++;
        }
    }
    if (!m_rslt.m_ov.resize(w, h)) {
        delete[] plain_dist_water;
        delete[] latitude;
        delete[] open_dist_water;
        delete[] dist_river;
        return false;
    }
    u8* climate = m_rslt.m_ov.data_w();
    std::memset(climate, CLIMATE_NONE, static_cast<size_t>(n));
    P1_ClimateTileVal* tiles = (tile_n > 0) ? new P1_ClimateTileVal[tile_n] : nullptr;
    if (tile_n > 0 && tiles == nullptr) {
        delete[] plain_dist_water;
        delete[] latitude;
        delete[] open_dist_water;
        delete[] dist_river;
        return false;
    }
    u32 ti = 0;
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            if (!is_open_land(terrain[i])) {
                continue;
            }
            P1_ClimateTileVal* e = &tiles[ti];
            e->m_x = px;
            e->m_y = py;
            e->m_val = tile_score(
                dist_river[i],
                open_dist_water[i],
                latitude[i],
                plain_dist_water[i],
                &m_sp.m_wts);
            ti++;
        }
    }
    if (tile_n > 0) {
        std::qsort(tiles, tile_n, sizeof(P1_ClimateTileVal), cmp_climate_tile);
        const u32 grass_n = pct_count(tile_n, m_sp.m_pct.m_pct_grassland);
        const u32 plains_n = pct_count(tile_n, m_sp.m_pct.m_pct_plains);
        for (u32 k = 0; k < tile_n; ++k) {
            const P1_ClimateTileVal* e = &tiles[k];
            const u32 idx = static_cast<u32>(e->m_y) * static_cast<u32>(w) + static_cast<u32>(e->m_x);
            u8 cls = CLIMATE_DESERT;
            if (k < grass_n) {
                cls = CLIMATE_GRASSLAND;
            } else if (k < grass_n + plains_n) {
                cls = CLIMATE_PLAINS;
            }
            climate[idx] = cls;
        }
    }
    assign_mountain_climate(w, h, terrain, climate);
    delete[] tiles;
    delete[] plain_dist_water;
    delete[] latitude;
    delete[] open_dist_water;
    delete[] dist_river;
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_Climate::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_ClimateRslt& P1_Gen_Climate::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
