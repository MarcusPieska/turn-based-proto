//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_gen_nearness_to_watershed_mtn.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static void bfs_land_to_border (
    u16 w,
    u16 h,
    const u8* terrain,
    const u16* border_ov,
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
        if (border_ov[i] == 0 || !is_land(terrain[i])) {
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

static u8 dissip_val (u16 d, u8 start_val, u8 fall_step) {
    if (d == k_inf || fall_step == 0) {
        return 0;
    }
    const u32 loss = static_cast<u32>(d) * static_cast<u32>(fall_step);
    const u32 sv = static_cast<u32>(start_val);
    if (loss >= sv) {
        return 0;
    }
    return static_cast<u8>(sv - loss);
}

static bool build_near_watershed_mtn (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_WatershedMountainLineSetsRslt& line_sets,
    u8 start_val,
    u8 fall_step,
    P1_Gen_NearnessToWatershedMtnRslt* out) 
{
    if (out == nullptr || terrain == nullptr || line_sets.m_ov == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* dist = new u16[n];
    if (dist == nullptr) {
        return false;
    }
    u16 max_dist = 0;
    bfs_land_to_border(w, h, terrain, line_sets.m_ov, dist, &max_dist);
    if (!out->m_ov.resize(w, h)) {
        delete[] dist;
        return false;
    }
    u8* pix = out->m_ov.data_w();
    for (u32 i = 0; i < n; ++i) {
        if (!is_land(terrain[i])) {
            pix[i] = 0;
            continue;
        }
        pix[i] = dissip_val(dist[i], start_val, fall_step);
    }
    delete[] dist;
    out->m_w = w;
    out->m_h = h;
    out->m_max_dist = max_dist;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_NearnessToWatershedMtn -
//================================================================================================================================

P1_Gen_NearnessToWatershedMtn::P1_Gen_NearnessToWatershedMtn (
    const P1_RunPrm& prm,
    const P1_Gen_NearnessToWatershedMtnPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_dist = 0;
}

bool P1_Gen_NearnessToWatershedMtn::generate (
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_WatershedMountainLineSetsRslt& line_sets) 
{
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_max_dist = 0;
    if (terrain == nullptr || w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (!build_near_watershed_mtn(terrain, w, h, line_sets, m_sp.m_start_val, m_sp.m_fall_step, &m_rslt)) {
        m_rslt.m_ov.clear();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_NearnessToWatershedMtn::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_NearnessToWatershedMtnRslt& P1_Gen_NearnessToWatershedMtn::result () const {
    return m_rslt;
}

void P1_Gen_NearnessToWatershedMtn::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_ov.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
