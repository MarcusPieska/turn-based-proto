//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_nearness_to_mtn.h"

#include "profile_time.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool is_mtn (u8 cls) {
    return cls == TERR_MOUNTAINS[0];
}

static bool is_flood_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static bool flood_near_mtn (
    const u8* terrain,
    u16 w,
    u16 h,
    u8 stop,
    P1_Gen_NearnessToMtnRslt* out) 
{
    if (out == nullptr || terrain == nullptr || !p1_map_size_ok(w, h) || stop == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    if (!out->m_ov.resize(w, h)) {
        return false;
    }
    u8* ov = out->m_ov.data_w();
    if (ov == nullptr) {
        return false;
    }
    std::memset(ov, 0, static_cast<size_t>(n));
    WB_QueXY q;
    if (!q.ok()) {
        out->m_ov.clear();
        return false;
    }
    q.clear();
    u32 seed_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_mtn(terrain[i])) {
            continue;
        }
        ov[i] = 1u;
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        if (!q.push(px, py)) {
            out->m_ov.clear();
            return false;
        }
        seed_n++;
    }
    while (q.count() > 0) {
        const u16 px = q.x_at(0);
        const u16 py = q.y_at(0);
        q.drop(1);
        const u32 i = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        const u8 d = static_cast<u8>(ov[i] - 1u);
        if (d >= stop) {
            continue;
        }
        const u8 nd = static_cast<u8>(d + 1u);
        const u8 mark = static_cast<u8>(nd + 1u);
        if (px > 0) {
            const u32 j = i - 1u;
            if (is_flood_land(terrain[j]) && ov[j] == 0) {
                ov[j] = mark;
                if (!q.push(static_cast<u16>(px - 1u), py)) {
                    out->m_ov.clear();
                    return false;
                }
            }
        }
        if (static_cast<u32>(px) + 1u < static_cast<u32>(w)) {
            const u32 j = i + 1u;
            if (is_flood_land(terrain[j]) && ov[j] == 0) {
                ov[j] = mark;
                if (!q.push(static_cast<u16>(px + 1u), py)) {
                    out->m_ov.clear();
                    return false;
                }
            }
        }
        if (py > 0) {
            const u32 j = i - static_cast<u32>(w);
            if (is_flood_land(terrain[j]) && ov[j] == 0) {
                ov[j] = mark;
                if (!q.push(px, static_cast<u16>(py - 1u))) {
                    out->m_ov.clear();
                    return false;
                }
            }
        }
        if (static_cast<u32>(py) + 1u < static_cast<u32>(h)) {
            const u32 j = i + static_cast<u32>(w);
            if (is_flood_land(terrain[j]) && ov[j] == 0) {
                ov[j] = mark;
                if (!q.push(px, static_cast<u16>(py + 1u))) {
                    out->m_ov.clear();
                    return false;
                }
            }
        }
    }
    u32 fill_n = 0;
    const u32 stop_u = static_cast<u32>(stop);
    for (u32 i = 0; i < n; ++i) {
        if (ov[i] == 0) {
            continue;
        }
        fill_n++;
        const u32 d = static_cast<u32>(ov[i] - 1u);
        ov[i] = static_cast<u8>((255u * (stop_u - d)) / stop_u);
    }
    out->m_w = w;
    out->m_h = h;
    out->m_stop = stop;
    out->m_seed_n = seed_n;
    out->m_fill_n = fill_n;
    return true;
}

//================================================================================================================================
//=> - P1_Gen_NearnessToMtn -
//================================================================================================================================

P1_Gen_NearnessToMtn::P1_Gen_NearnessToMtn (const P1_RunPrm& prm, const P1_Gen_NearnessToMtnPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_stop = 0;
    m_rslt.m_seed_n = 0;
    m_rslt.m_fill_n = 0;
}

bool P1_Gen_NearnessToMtn::generate (const u8* terrain, u16 w, u16 h) {
    PTIME_START(__func__);
    m_valid_generation = false;
    m_rslt.m_ov.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_stop = 0;
    m_rslt.m_seed_n = 0;
    m_rslt.m_fill_n = 0;
    if (terrain == nullptr || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return false;
    }
    u8 stop = m_sp.m_stop;
    if (stop == 0) {
        stop = p1_gen_nearness_to_mtn_stop_from_w(w);
    }
    if (!flood_near_mtn(terrain, w, h, stop, &m_rslt)) {
        m_rslt.m_ov.clear();
        PTIME_STOP(__func__);
        return false;
    }
    m_valid_generation = true;
    PTIME_STOP(__func__);
    return true;
}

bool P1_Gen_NearnessToMtn::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_NearnessToMtnRslt& P1_Gen_NearnessToMtn::result () const {
    return m_rslt;
}

void P1_Gen_NearnessToMtn::save_output (cstr path) const {
    if (!m_valid_generation || path == nullptr) {
        return;
    }
    m_rslt.m_ov.save(path);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
