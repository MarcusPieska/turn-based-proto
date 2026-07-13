//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "p1_adj_river_inlets.h"
#include "generator_constants.h"
#include "p1_gen_river_network.h"

//================================================================================================================================
//=> - Inlet sys helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dx8[8] = {-1, 1, 0, 0, -1, 1, -1, 1};
static const i32 k_dy8[8] = {0, 0, -1, 1, -1, -1, 1, 1};
static const u16 k_sys_none = 0xFFFFu;

struct RivSysEnt {
    u16 m_mx;
    u16 m_my;
    u16 m_max_d;
    u32 m_tile_n;
};

static u32 tidx (u16 w, i32 x, i32 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w) && static_cast<u32>(y) < static_cast<u32>(h);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

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

static void flood_wat_comp (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u32 seed,
    u8* wat,
    u32* mouths,
    u32* mouth_n,
    u32 mouth_cap,
    u32* q,
    u32 qcap) 
{
    if (wat[seed] != 0) {
        return;
    }
    u32 qn = 0;
    wat[seed] = 1;
    q[qn++] = seed;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (riv[ni] != 0) {
                if (*mouth_n < mouth_cap) {
                    mouths[(*mouth_n)++] = ni;
                }
                continue;
            }
            if (!is_water(terrain[ni]) || wat[ni] != 0 || qn >= qcap) {
                continue;
            }
            wat[ni] = 1;
            q[qn++] = ni;
        }
    }
}

static u16 mouth_basin (const u16* basin, const u8* riv, u16 w, u16 h, u16 mx, u16 my) {
    const u32 si = tidx(w, mx, my);
    if (basin[si] != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
        return basin[si];
    }
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(mx) + k_dx8[k];
        const i32 ny = static_cast<i32>(my) + k_dy8[k];
        if (!in_map(w, h, nx, ny)) {
            continue;
        }
        const u32 ni = tidx(w, nx, ny);
        if (riv[ni] != 0 && basin[ni] != static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            return basin[ni];
        }
    }
    return static_cast<u16>(P1_RIVER_BASIN_NONE);
}

static void trace_sys (
    u16 w,
    u16 h,
    const u8* riv,
    const u16* basin,
    u16 mx,
    u16 my,
    u16 basin_id,
    u16 sys_ix,
    u16* rdep,
    u16* rsys,
    RivSysEnt* ent) 
{
    ent->m_mx = mx;
    ent->m_my = my;
    ent->m_max_d = 0;
    ent->m_tile_n = 0;
    const u32 si = tidx(w, mx, my);
    if (riv[si] == 0 || rdep[si] != 0) {
        return;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32* q = new u32[n];
    if (q == nullptr) {
        return;
    }
    u32 qn = 0;
    rdep[si] = 1;
    rsys[si] = sys_ix;
    ent->m_tile_n = 1;
    ent->m_max_d = 1;
    q[qn++] = si;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 d = rdep[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 8; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx8[k];
            const i32 ny = static_cast<i32>(py) + k_dy8[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, nx, ny);
            if (riv[ni] == 0 || rdep[ni] != 0 || basin[ni] != basin_id) {
                continue;
            }
            const u16 nd = static_cast<u16>(d + 1u);
            rdep[ni] = nd;
            rsys[ni] = sys_ix;
            ent->m_tile_n++;
            if (nd > ent->m_max_d) {
                ent->m_max_d = nd;
            }
            q[qn++] = ni;
        }
    }
    delete[] q;
}

static void sort_sys (RivSysEnt* sys, u16* rsys, u16 sys_n, u16 w, u16 h) {
    if (sys_n <= 1u) {
        return;
    }
    u16* ord = new u16[sys_n];
    u16* remap = new u16[sys_n];
    RivSysEnt* tmp = new RivSysEnt[sys_n];
    if (ord == nullptr || remap == nullptr || tmp == nullptr) {
        delete[] tmp;
        delete[] remap;
        delete[] ord;
        return;
    }
    for (u16 i = 0; i < sys_n; ++i) {
        ord[i] = i;
    }
    for (u16 i = 1; i < sys_n; ++i) {
        u16 j = i;
        while (j > 0) {
            const RivSysEnt& a = sys[ord[j]];
            const RivSysEnt& b = sys[ord[j - 1]];
            const bool gt = (a.m_tile_n != b.m_tile_n) ? (a.m_tile_n > b.m_tile_n)
                : ((a.m_max_d != b.m_max_d) ? (a.m_max_d > b.m_max_d) : (ord[j] < ord[j - 1]));
            if (!gt) {
                break;
            }
            const u16 t = ord[j];
            ord[j] = ord[j - 1];
            ord[j - 1] = t;
            --j;
        }
    }
    for (u16 i = 0; i < sys_n; ++i) {
        tmp[i] = sys[ord[i]];
        remap[ord[i]] = i;
    }
    for (u16 i = 0; i < sys_n; ++i) {
        sys[i] = tmp[i];
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (rsys[i] != k_sys_none && rsys[i] < sys_n) {
            rsys[i] = remap[rsys[i]];
        }
    }
    delete[] tmp;
    delete[] remap;
    delete[] ord;
}

static bool build_riv_sys (
    const u8* terrain,
    const u8* riv,
    const u16* basin,
    u16 w,
    u16 h,
    u16** rdep_out,
    u16** rsys_out,
    RivSysEnt** sys_out,
    u16* sys_n_out) 
{
    *rdep_out = nullptr;
    *rsys_out = nullptr;
    *sys_out = nullptr;
    *sys_n_out = 0;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u16* rdep = new u16[n];
    u16* rsys = new u16[n];
    if (rdep == nullptr || rsys == nullptr) {
        delete[] rsys;
        delete[] rdep;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        rdep[i] = 0;
        rsys[i] = k_sys_none;
    }
    u8* wat = new u8[n];
    u32* mouths = new u32[n];
    u32* q = new u32[n];
    RivSysEnt* tmp = new RivSysEnt[n];
    if (wat == nullptr || mouths == nullptr || q == nullptr || tmp == nullptr) {
        delete[] tmp;
        delete[] q;
        delete[] mouths;
        delete[] wat;
        delete[] rsys;
        delete[] rdep;
        return false;
    }
    std::memset(wat, 0, n);
    u32 mouth_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (is_water(terrain[i]) && wat[i] == 0) {
            flood_wat_comp(terrain, riv, w, h, i, wat, mouths, &mouth_n, n, q, n);
        }
    }
    u16 sys_n = 0;
    for (u32 mi = 0; mi < mouth_n; ++mi) {
        const u32 i = mouths[mi];
        if (rdep[i] != 0) {
            continue;
        }
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 basin_id = mouth_basin(basin, riv, w, h, px, py);
        if (basin_id == static_cast<u16>(P1_RIVER_BASIN_NONE)) {
            continue;
        }
        RivSysEnt ent = {};
        trace_sys(w, h, riv, basin, px, py, basin_id, sys_n, rdep, rsys, &ent);
        if (ent.m_tile_n > 0u) {
            tmp[sys_n++] = ent;
        }
    }
    delete[] q;
    delete[] mouths;
    delete[] wat;
    if (sys_n == 0u) {
        delete[] tmp;
        delete[] rsys;
        delete[] rdep;
        return false;
    }
    RivSysEnt* sys = new RivSysEnt[sys_n];
    if (sys == nullptr) {
        delete[] tmp;
        delete[] rsys;
        delete[] rdep;
        return false;
    }
    for (u16 i = 0; i < sys_n; ++i) {
        sys[i] = tmp[i];
    }
    delete[] tmp;
    sort_sys(sys, rsys, sys_n, w, h);
    *rdep_out = rdep;
    *rsys_out = rsys;
    *sys_out = sys;
    *sys_n_out = sys_n;
    return true;
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
    u8* riv,
    const u16* basin) 
{
    m_valid_adjust = false;
    if (terrain == nullptr || riv == nullptr || basin == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    if (w != m_prm.m_w || h != m_prm.m_h) {
        return false;
    }
    if (m_sp.m_perc == 0 || m_sp.m_min_sz == 0) {
        return false;
    }
    u16* rdep = nullptr;
    u16* rsys = nullptr;
    RivSysEnt* sys = nullptr;
    u16 sys_n = 0;
    if (!build_riv_sys(terrain, riv, basin, w, h, &rdep, &rsys, &sys, &sys_n)) {
        delete[] sys;
        delete[] rsys;
        delete[] rdep;
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || rdep[i] == 0) {
            continue;
        }
        const u16 si = rsys[i];
        if (si >= sys_n) {
            continue;
        }
        const u16 lim = inlet_lim(sys[si].m_max_d, m_sp.m_perc, m_sp.m_min_sz);
        if (rdep[i] <= lim) {
            terrain[i] = TERR_COASTAL[0];
            riv[i] = 0u;
        }
    }
    delete[] sys;
    delete[] rsys;
    delete[] rdep;
    m_valid_adjust = true;
    return true;
}

bool P1_Adj_RiverInlets::is_valid () const {
    return m_valid_adjust;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
