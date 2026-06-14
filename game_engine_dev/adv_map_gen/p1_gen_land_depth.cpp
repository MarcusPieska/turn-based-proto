//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_land_depth.h"
#include "p1_gen_outline.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const u8 k_disp_water[3] = {30, 110, 220};

static inline bool wl_is_water (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_WATER_GRAY;
}

static inline bool wl_is_land (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_LAND_GRAY;
}

static void bfs_land_depth (u16 wi, u16 hi, const u8* wl, u16* out_dist) {
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi);
    for (u32 i = 0; i < n; ++i) {
        out_dist[i] = k_inf;
    }
    const u32 qcap = n;
    u32* q = new u32[qcap];
    if (q == nullptr) {
        return;
    }
    size_t qh = 0;
    size_t qn = 0;
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
                q[qn++] = i;
            }
        }
    }
    while (qh < qn) {
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
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(wi)) {
            const u32 j = i + 1u;
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q[qn++] = j;
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q[qn++] = j;
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(hi)) {
            const u32 j = i + static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                q[qn++] = j;
            }
        }
    }
    delete[] q;
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
//=> - P1_Gen_LandDepth -
//================================================================================================================================

P1_Gen_LandDepth::P1_Gen_LandDepth (const P1_RunPrm& prm) :
    m_prm(prm),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
}

bool P1_Gen_LandDepth::generate (const u8* ol_gray, u16 w, u16 h) {
    m_valid_generation = false;
    m_rslt.m_wl.clear();
    m_rslt.m_dist.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    if (ol_gray == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if (!m_rslt.m_wl.assign_copy(w, h, ol_gray) || !m_rslt.m_dist.resize(w, h)) {
        return false;
    }
    bfs_land_depth(w, h, m_rslt.m_wl.data(), m_rslt.m_dist.data_w());
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_LandDepth::generate () {
    P1_Gen_Outline ol(m_prm);
    if (!ol.generate() || !ol.is_valid()) {
        return false;
    }
    const P1_Gen_OutlineRslt& r = ol.result();
    const u8* ov = r.m_ov.data();
    if (ov == nullptr) {
        return false;
    }
    return generate(ov, r.m_w, r.m_h);
}

bool P1_Gen_LandDepth::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_LandDepthRslt& P1_Gen_LandDepth::result () const {
    return m_rslt;
}

void P1_Gen_LandDepth::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_dist.width();
    const u16 h = m_rslt.m_dist.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u8* wl = m_rslt.m_wl.data();
    const u16* dist = m_rslt.m_dist.data();
    if (wl == nullptr || dist == nullptr || n == 0) {
        return;
    }
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
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8* o = rgb + i * 3u;
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
    save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
