//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_land_depth.h"

#include "p1_gen_cont_outlines.h"
#include "p1_step_log.h"
#include "p1_wb_util.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static inline bool wl_is_water (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_WATER_GRAY;
}

static inline bool wl_is_land (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_LAND_GRAY;
}

static bool bfs_land_depth (u16 wi, u16 hi, const u8* wl, u16* out_dist) {
    const u32 n = static_cast<u32>(wi) * static_cast<u32>(hi); 
    for (u32 i = 0; i < n; ++i) {
        out_dist[i] = k_inf;
    }
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
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
                que.push(px, py);
            }
        }
    }
    u32 qi = 0u;
    while (qi < que.count()) {
        const u16 px = que.x_at(qi);
        const u16 py = que.y_at(qi);
        qi++;
        const u32 i = static_cast<u32>(py) * static_cast<u32>(wi) + static_cast<u32>(px);
        const u16 cur = out_dist[i];
        if (cur == k_inf || cur >= 65534u) {
            continue;
        }
        const u16 next = static_cast<u16>(cur + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                const u16 nx = static_cast<u16>(px - 1u);
                que.push(nx, py);
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(wi)) {
            const u32 j = i + 1u;
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                const u16 nx = static_cast<u16>(px + 1u);
                que.push(nx, py);
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                const u16 ny = static_cast<u16>(py - 1u);
                que.push(px, ny);
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(hi)) {
            const u32 j = i + static_cast<u32>(wi);
            if (wl_is_land(wl, j) && out_dist[j] == k_inf) {
                out_dist[j] = next;
                const u16 ny = static_cast<u16>(py + 1u);
                que.push(px, ny);
            }
        }
    }
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
        P1_STEP_ABORT("P1_Gen_LandDepth", "null overlay or invalid map size");
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!m_rslt.m_dist.resize(w, h)) {
        P1_STEP_ABORT("P1_Gen_LandDepth", "buffer alloc failed");
    }
    Whiteboard_1B wb_wl("P1_Gen_LandDepth", "wl", m_prm.m_seed);
    P1_WB_CHK(wb_wl);
    std::memcpy(wb_wl.raw(), ol_gray, n);
    if (!bfs_land_depth(w, h, wb_wl.raw(), m_rslt.m_dist.data_w())) {
        P1_STEP_ABORT("P1_Gen_LandDepth", "bfs land depth failed");
    }
    if (!m_rslt.m_wl.assign_copy(w, h, wb_wl.raw())) {
        P1_STEP_ABORT("P1_Gen_LandDepth", "wl copy failed");
    }
    m_rslt.m_w = w;
    m_rslt.m_h = h;
    m_valid_generation = true;
    return true;
}

bool P1_Gen_LandDepth::generate () {
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(m_prm, &ov_map)) {
        P1_STEP_ABORT("P1_Gen_LandDepth", "step01 outline failed");
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr) {
        P1_STEP_ABORT("P1_Gen_LandDepth", "step01 outline null data");
    }
    return generate(ov, ov_map.width(), ov_map.height());
}

bool P1_Gen_LandDepth::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_LandDepthRslt& P1_Gen_LandDepth::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
