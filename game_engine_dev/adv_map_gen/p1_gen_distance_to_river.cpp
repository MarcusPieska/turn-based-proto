//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_gen_distance_to_river.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static void bfs_land_to_river (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* river,
    u16* dist,
    u16* max_dist) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    *max_dist = 0;
    for (u32 i = 0; i < n; ++i) {
        dist[i] = k_inf;
    }
    const u32 qcap = n;
    u32* q = new u32[qcap];
    if (q == nullptr) {
        return;
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
    delete[] q;
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

static bool build_dist_river (
    const u8* terrain,
    u16 w,
    u16 h,
    const u8* river_ov,
    P1_Gen_DistanceToRiverRslt* out) 
{
    if (out == nullptr || terrain == nullptr || river_ov == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return false;
    }
    u16 max_dist = 0;
    bfs_land_to_river(w, h, terrain, river_ov, dist, &max_dist);
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
    if (!out->m_ov.resize(w, h)) {
        delete[] dist;
        return false;
    }
    u8* pix = out->m_ov.data_w();
    for (u32 i = 0; i < n; ++i) {
        const u8 cls = terrain[i];
        if (is_water(cls)) {
            pix[i] = 0;
            continue;
        }
        if (river_ov[i] != 0) {
            pix[i] = 0;
            continue;
        }
        const u16 d = dist[i];
        if (!is_land(cls) || d == k_inf) {
            pix[i] = 0;
            continue;
        }
        pix[i] = scale_dist(d, max_dist, land_all_zero);
    }
    delete[] dist;
    out->m_w = w;
    out->m_h = h;
    out->m_max_dist = max_dist;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_DistanceToRiver -
//================================================================================================================================

P1_Gen_DistanceToRiver::P1_Gen_DistanceToRiver (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_dist = 0;
}

bool P1_Gen_DistanceToRiver::generate (const u8* terrain, u16 w, u16 h, const u8* river_ov) {
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_dist = 0;
    if (terrain == nullptr || river_ov == nullptr || w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_dist_river(terrain, w, h, river_ov, &m_rslt)) {
        m_rslt.m_ov.clear();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_DistanceToRiver::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_DistanceToRiverRslt& P1_Gen_DistanceToRiver::result () const {
    return m_rslt;
}

void P1_Gen_DistanceToRiver::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_ov.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
