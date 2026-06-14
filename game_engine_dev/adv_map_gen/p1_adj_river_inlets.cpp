//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_river_inlets.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private adjustment helpers -
//================================================================================================================================

static u16 inlet_lim (u16 max_d, u8 perc, u8 min_sz) {
    if (max_d == 0) {
        return 0;
    }
    u32 lim = (static_cast<u32>(max_d) * static_cast<u32>(perc) + 99u) / 100u;
    if (lim < static_cast<u32>(min_sz)) {
        lim = min_sz;
    }
    if (lim > static_cast<u32>(max_d)) {
        lim = max_d;
    }
    return static_cast<u16>(lim);
}

//================================================================================================================================
//=> - P1_Adj_RiverInlets -
//================================================================================================================================

P1_Adj_RiverInlets::P1_Adj_RiverInlets (const P1_RunPrm& prm, const P1_Adj_RiverInletsPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_adjust(false) {
}

bool P1_Adj_RiverInlets::adjust (
    u8* terrain,
    u16 w,
    u16 h,
    const u8* riv,
    const P1_Gen_RiverLinesRslt& lines) 
{
    m_valid_adjust = false;
    if (terrain == nullptr || riv == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (lines.m_sys == nullptr || lines.m_rdep == nullptr || lines.m_rsys == nullptr
        || lines.m_w == 0 || lines.m_h == 0 || lines.m_sys_n == 0) {
        return false;
    }
    if (m_sp.m_perc == 0 || m_sp.m_min_sz == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || lines.m_rdep[i] == 0) {
            continue;
        }
        const u16 si = lines.m_rsys[i];
        if (si >= lines.m_sys_n) {
            continue;
        }
        const u16 lim = inlet_lim(lines.m_sys[si].m_max_d, m_sp.m_perc, m_sp.m_min_sz);
        if (lines.m_rdep[i] <= lim) {
            terrain[i] = TERR_COASTAL[0];
        }
    }
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_RiverInlets::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
