//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstddef>
#include <vector>

#include "generate_generic_depth_overlay.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static bool is_tgt_eq (u8 cls, u8 terr_idx) {
    return cls == terr_idx;
}

static bool is_tgt_ge (u8 cls, u8 terr_idx) {
    return cls >= terr_idx;
}

static void bfs_terr_depth (
    u16 w,
    u16 h,
    const u8* terrain,
    u8 terr_idx,
    bool ge,
    u16* dist,
    u16* max_dist) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 wi = static_cast<u32>(w);
    *max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    std::vector<u32> q;
    q.reserve(static_cast<std::size_t>(n / 4u + 64u));
    for (u16 py = 0; py < h; ++py) {
        for (u16 px = 0; px < w; ++px) {
            const u32 i = static_cast<u32>(py) * wi + static_cast<u32>(px);
            const bool tgt = ge ? is_tgt_ge(terrain[i], terr_idx) : is_tgt_eq(terrain[i], terr_idx);
            if (!tgt) {
                continue;
            }
            bool adj = false;
            if (px > 0) {
                const bool nb = ge ? is_tgt_ge(terrain[i - 1u], terr_idx) : is_tgt_eq(terrain[i - 1u], terr_idx);
                if (!nb) {
                    adj = true;
                }
            }
            if (!adj && px + 1u < w) {
                const bool nb = ge ? is_tgt_ge(terrain[i + 1u], terr_idx) : is_tgt_eq(terrain[i + 1u], terr_idx);
                if (!nb) {
                    adj = true;
                }
            }
            if (!adj && py > 0) {
                const bool nb = ge ? is_tgt_ge(terrain[i - wi], terr_idx) : is_tgt_eq(terrain[i - wi], terr_idx);
                if (!nb) {
                    adj = true;
                }
            }
            if (!adj && py + 1u < h) {
                const bool nb = ge ? is_tgt_ge(terrain[i + wi], terr_idx) : is_tgt_eq(terrain[i + wi], terr_idx);
                if (!nb) {
                    adj = true;
                }
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
        const u16 py = static_cast<u16>(i / wi);
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * wi);
        const u16 nxt = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            const bool ok = ge ? is_tgt_ge(terrain[j], terr_idx) : is_tgt_eq(terrain[j], terr_idx);
            if (ok && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < wi) {
            const u32 j = i + 1u;
            const bool ok = ge ? is_tgt_ge(terrain[j], terr_idx) : is_tgt_eq(terrain[j], terr_idx);
            if (ok && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - wi;
            const bool ok = ge ? is_tgt_ge(terrain[j], terr_idx) : is_tgt_eq(terrain[j], terr_idx);
            if (ok && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + wi;
            const bool ok = ge ? is_tgt_ge(terrain[j], terr_idx) : is_tgt_eq(terrain[j], terr_idx);
            if (ok && dist[j] == k_inf) {
                dist[j] = nxt;
                q.push_back(j);
            }
        }
    }
}

static u8 scale_dist (u16 d, u16 max_dist, bool all_zero);

static u8* finish_depth (
    const u8* terrain,
    u16 w,
    u16 h,
    u8 terr_idx,
    bool ge,
    u16* dist) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16 max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        if (dist[i] != k_inf && dist[i] > max_dist) {
            max_dist = dist[i];
        }
    }
    u32 tgt_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        const bool tgt = ge ? is_tgt_ge(terrain[i], terr_idx) : is_tgt_eq(terrain[i], terr_idx);
        if (!tgt) {
            continue;
        }
        if (dist[i] != k_inf) {
            tgt_fin++;
        }
    }
    const bool all_zero = (max_dist == 0 && tgt_fin > 0);
    if (max_dist == 0 && !all_zero) {
        max_dist = 1;
    }
    u8* out = new u8[n];
    if (out == nullptr) {
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        const bool tgt = ge ? is_tgt_ge(terrain[i], terr_idx) : is_tgt_eq(terrain[i], terr_idx);
        if (!tgt) {
            out[i] = 0;
            continue;
        }
        const u16 d = dist[i];
        if (d == k_inf) {
            out[i] = 0;
            continue;
        }
        out[i] = scale_dist(d, max_dist, all_zero);
    }
    return out;
}

static u8 scale_dist (u16 d, u16 max_dist, bool all_zero) {
    if (all_zero) {
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
//=> - Generate_GenericDepthOverlay -
//================================================================================================================================

u8* Generate_GenericDepthOverlay::generate (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return nullptr;
    }
    u16 max_dist = 0;
    bfs_terr_depth(w, h, terrain, terr_idx, false, dist, &max_dist);
    u8* out = finish_depth(terrain, w, h, terr_idx, false, dist);
    delete[] dist;
    return out;
}

u8* Generate_GenericDepthOverlay::generate_ge (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return nullptr;
    }
    u16 max_dist = 0;
    bfs_terr_depth(w, h, terrain, terr_idx, true, dist, &max_dist);
    u8* out = finish_depth(terrain, w, h, terr_idx, true, dist);
    delete[] dist;
    return out;
}

u16* Generate_GenericDepthOverlay::generate_ge_dist (const u8* terrain, u16 w, u16 h, u8 terr_idx) {
    if (terrain == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return nullptr;
    }
    u16 max_dist = 0;
    bfs_terr_depth(w, h, terrain, terr_idx, true, dist, &max_dist);
    return dist;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
