//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>
#include <vector>

#include "generate_distance_land_to_water.h"
#include "generate_overlay_water_land.h"
#include "generate_terrain_cont_pn.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const u8 k_disp_water[3] = {30, 110, 220};
static const u8 k_disp_land[3] = {50, 170, 70};

static inline bool wl_is_water (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_WATER_GRAY;
}

static inline bool wl_is_land (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_LAND_GRAY;
}

static void bfs_land_distance_to_water (u16 wi, u16 hi, const u8* wl, u16* out_dist) {
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi);
    for (u32 i = 0; i < n; ++i) {
        out_dist[i] = k_inf;
    }
    std::vector<u32> q;
    q.reserve(static_cast<size_t>(n / 4u + 64u));
    for (u16 py = 0; py < hi; ++py) {
        for (u16 px = 0; px < wi; ++px) {
            const u32 i = static_cast<u32>(py) * static_cast<u32>(wi) + static_cast<u32>(px);
            if (!wl_is_land(wl, i)) {
                continue;
            }
            bool adj = false;
            if (px > 0 && wl_is_water(wl, i - 1u)) {
                adj = true;
            }
            if (!adj && static_cast<u32>(px) + 1u < static_cast<u32>(wi) && wl_is_water(wl, i + 1u)) {
                adj = true;
            }
            if (!adj && py > 0 && wl_is_water(wl, i - static_cast<u32>(wi))) {
                adj = true;
            }
            if (!adj && static_cast<u32>(py) + 1u < static_cast<u32>(hi) && wl_is_water(wl, i + static_cast<u32>(wi))) {
                adj = true;
            }
            if (adj) {
                out_dist[i] = 0;
                q.push_back(i);
            }
        }
    }
    size_t qh = 0;
    while (qh < q.size()) {
        const u32 i = q[qh++];
        const u16 cur = out_dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        const u16 py = static_cast<u16>(i / static_cast<u32>(wi));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(wi));
        const u16 next = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(wi)) {
            const u32 j = i + 1u;
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q.push_back(j);
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q.push_back(j);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(hi)) {
            const u32 j = i + static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q.push_back(j);
            }
        }
    }
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

//================================================================================================================================
//=> - DistanceLandToWater -
//================================================================================================================================

Generate_DistanceLandToWater::Generate_DistanceLandToWater (u32 seed) :
    m_seed(seed),
    m_valid_generation(false),
    m_wl(),
    m_dist() {
}

bool Generate_DistanceLandToWater::generate (const u8* wl_gray, u16 w, u16 h) {
    m_valid_generation = false;
    m_wl.clear();
    m_dist.clear();
    if (wl_gray == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (!m_wl.assign_copy(w, h, wl_gray) || !m_dist.resize(w, h)) {
        return false;
    }
    bfs_land_distance_to_water(w, h, m_wl.data(), m_dist.data_w());
    m_valid_generation = true;
    return true;
}

bool Generate_DistanceLandToWater::generate () {
    TerrainContPnParams tp;
    tp.m_seed = m_seed;
    Generate_TerrainContPn terr(tp);
    if (!terr.is_valid()) {
        return false;
    }
    Generate_OverlayWaterLand wl;
    if (!wl.generate(terr.terrain()) || !wl.is_valid()) {
        return false;
    }
    const u8* g = wl.water_land_gray();
    if (g == nullptr) {
        return false;
    }
    return generate(g, wl.width(), wl.height());
}

bool Generate_DistanceLandToWater::is_valid () const {
    return m_valid_generation;
}

bool Generate_DistanceLandToWater::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return false;
    }
    const u16 w = m_dist.width();
    const u16 h = m_dist.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u8* wl = m_wl.data();
    const u16* dist = m_dist.data();
    u32 max_vis = 0;
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!wl_is_land(wl, i)) {
            continue;
        }
        const u16 d = dist[i];
        if (d == k_inf) {
            continue;
        }
        land_fin++;
        const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
        if (dv > max_vis) {
            max_vis = dv;
        }
    }
    const bool land_all_zero = (max_vis == 0u && land_fin > 0u);
    if (max_vis == 0u) {
        max_vis = 1u;
    }
    std::vector<u8> rgb(static_cast<size_t>(n) * 3u);
    for (u32 i = 0; i < n; ++i) {
        u8* o = rgb.data() + i * 3u;
        if (wl_is_water(wl, i)) {
            std::memcpy(o, k_disp_water, 3u);
            continue;
        }
        const u16 d = dist[i];
        u8 t = 0;
        if (d != k_inf) {
            if (land_all_zero) {
                t = 255;
            } else {
                const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
                t = static_cast<u8>((dv * 255u) / max_vis);
            }
        }
        o[0] = t;
        o[1] = t;
        o[2] = t;
    }
    return save_rgb_ppm(path, rgb.data(), w, h);
}

u16 Generate_DistanceLandToWater::width () const {
    return m_dist.width();
}

u16 Generate_DistanceLandToWater::height () const {
    return m_dist.height();
}

const MapArrayDistance& Generate_DistanceLandToWater::distance () const {
    return m_dist;
}

const MapArrayOverlay& Generate_DistanceLandToWater::water_land () const {
    return m_wl;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
