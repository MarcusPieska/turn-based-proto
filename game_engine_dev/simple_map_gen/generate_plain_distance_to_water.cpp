//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_plain_distance_to_water.h"

#include "generator_constants.h"

#include <cstddef>
#include <vector>

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static void bfs_land_to_water (u16 w, u16 h, const u8* terrain, u16* dist, u16* max_dist) {
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    std::vector<u32> q;
    q.reserve(static_cast<std::size_t>(n / 4u + 64u));
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
            const u8 cls = terrain[i];
            if (!is_land(cls)) {
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
                q.push_back(i);
            }
        }
    }
    std::size_t qh = 0;
    while (qh < q.size()) {
        const u32 i = q[qh++];
        const u16 cur = dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        if (cur > *max_dist) {
            *max_dist = cur;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_land(terrain[j]) && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
    }
}

static u8 scale_dist (u16 d, u16 max_dist, bool land_all_zero) {
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

//================================================================================================================================
//=> - Generate_PlainDistanceToWater -
//================================================================================================================================

u8* Generate_PlainDistanceToWater::generate (const u8* terrain, u16 w, u16 h) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return nullptr;
    }
    u16 max_dist = 0;
    bfs_land_to_water(w, h, terrain, dist, &max_dist);
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terrain[i])) {
            continue;
        }
        if (dist[i] == k_inf) {
            continue;
        }
        land_fin++;
    }
    const bool land_all_zero = (max_dist == 0 && land_fin > 0);
    if (max_dist == 0 && !land_all_zero) {
        max_dist = 1;
    }
    u8* out = new u8[n];
    if (out == nullptr) {
        delete[] dist;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = terrain[i];
        if (is_water(cls)) {
            out[i] = 0;
            continue;
        }
        const u16 d = dist[i];
        if (!is_land(cls) || d == k_inf) {
            out[i] = 0;
            continue;
        }
        out[i] = scale_dist(d, max_dist, land_all_zero);
    }
    delete[] dist;
    return out;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
