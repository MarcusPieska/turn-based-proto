//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "generate_river_line_data.h"

#include <algorithm>
#include <cstring>
#include <vector>

#include "generate_river_network.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};
static const i32 k_dxd[4] = {-1, 1, -1, 1};
static const i32 k_dyd[4] = {-1, -1, 1, 1};

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static void flood_wat_comp (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u32 seed,
    u8* wat,
    std::vector<u32>* mouths) 
{
    if (wat[seed] != 0) {
        return;
    }
    std::vector<u32> q;
    wat[seed] = 1;
    q.push_back(seed);
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (riv[ni] != 0) {
                mouths->push_back(ni);
                continue;
            }
            if (!is_water(terrain[ni]) || wat[ni] != 0) {
                continue;
            }
            wat[ni] = 1;
            q.push_back(ni);
        }
    }
}

static u32 flood_all_water (
    const u8* terrain,
    const u8* riv,
    u16 w,
    u16 h,
    u8* wat,
    std::vector<u32>* mouths) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(wat, 0, n);
    mouths->clear();
    u32 comp_n = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!is_water(terrain[i]) || wat[i] != 0) {
            continue;
        }
        flood_wat_comp(terrain, riv, w, h, i, wat, mouths);
        comp_n++;
    }
    return comp_n;
}

static u16 mouth_basin (
    const u16* basin,
    const u8* riv,
    u16 w,
    u16 h,
    u16 mx,
    u16 my) 
{
    const u32 si = tidx(w, mx, my);
    if (basin[si] != static_cast<u16>(RIVER_BASIN_NONE)) {
        return basin[si];
    }
    for (i32 k = 0; k < 8; ++k) {
        const i32 nx = static_cast<i32>(mx) + ((k < 4) ? k_dx4[k] : k_dxd[k - 4]);
        const i32 ny = static_cast<i32>(my) + ((k < 4) ? k_dy4[k] : k_dyd[k - 4]);
        if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
            continue;
        }
        const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
        if (riv[ni] == 0 || basin[ni] == static_cast<u16>(RIVER_BASIN_NONE)) {
            continue;
        }
        return basin[ni];
    }
    return static_cast<u16>(RIVER_BASIN_NONE);
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
    RiverSysEntry* ent) 
{
    ent->mx = mx;
    ent->my = my;
    ent->max_d = 0;
    ent->tile_n = 0;
    const u32 si = tidx(w, mx, my);
    if (riv[si] == 0 || rdep[si] != 0) {
        return;
    }
    std::vector<u32> q;
    rdep[si] = 1;
    rsys[si] = sys_ix;
    ent->tile_n = 1;
    ent->max_d = 1;
    q.push_back(si);
    for (std::size_t qi = 0; qi < q.size(); ++qi) {
        const u32 i = q[qi];
        const u16 d = rdep[i];
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 px = static_cast<u16>(i - static_cast<u32>(py) * static_cast<u32>(w));
        for (i32 k = 0; k < 8; ++k) {
            const i32 nx = static_cast<i32>(px) + ((k < 4) ? k_dx4[k] : k_dxd[k - 4]);
            const i32 ny = static_cast<i32>(py) + ((k < 4) ? k_dy4[k] : k_dyd[k - 4]);
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= static_cast<u32>(w) || static_cast<u32>(ny) >= static_cast<u32>(h)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (riv[ni] == 0 || rdep[ni] != 0 || basin[ni] != basin_id) {
                continue;
            }
            const u16 nd = static_cast<u16>(d + 1u);
            rdep[ni] = nd;
            rsys[ni] = sys_ix;
            ent->tile_n++;
            if (nd > ent->max_d) {
                ent->max_d = nd;
            }
            q.push_back(ni);
        }
    }
}

struct SysSortRec {
    u16 old_ix;
    RiverSysEntry ent;
};

static bool sys_sort_cmp (const SysSortRec& a, const SysSortRec& b) {
    if (a.ent.tile_n != b.ent.tile_n) {
        return a.ent.tile_n > b.ent.tile_n;
    }
    if (a.ent.max_d != b.ent.max_d) {
        return a.ent.max_d > b.ent.max_d;
    }
    return a.old_ix < b.old_ix;
}

static void sort_sys (
    u16 w,
    u16 h,
    RiverSysEntry* sys,
    u16* rsys,
    u16 sys_n) 
{
    if (sys == nullptr || rsys == nullptr || sys_n == 0) {
        return;
    }
    std::vector<SysSortRec> recs;
    recs.reserve(sys_n);
    for (u16 i = 0; i < sys_n; ++i) {
        SysSortRec r = {};
        r.old_ix = i;
        r.ent = sys[i];
        recs.push_back(r);
    }
    std::sort(recs.begin(), recs.end(), sys_sort_cmp);
    std::vector<u16> remap(sys_n);
    for (u16 i = 0; i < sys_n; ++i) {
        sys[i] = recs[i].ent;
        remap[recs[i].old_ix] = i;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    for (u32 i = 0; i < n; ++i) {
        if (rsys[i] == RIVER_LINE_SYS_NONE) {
            continue;
        }
        rsys[i] = remap[rsys[i]];
    }
}

//================================================================================================================================
//=> - Generate_RiverLineData -
//================================================================================================================================

RiverLineDataResult* Generate_RiverLineData::generate (const u8* terrain, const u8* riv, const u16* basin, u16 w, u16 h) {
    if (terrain == nullptr || riv == nullptr || basin == nullptr || w == 0 || h == 0) {
        return nullptr;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    RiverLineDataResult* out = new RiverLineDataResult();
    if (out == nullptr) {
        return nullptr;
    }
    out->w = w;
    out->h = h;
    out->sys_n = 0;
    out->sys = nullptr;
    out->rdep = new u16[n];
    out->rsys = new u16[n];
    u8* wat = new u8[n];
    if (out->rdep == nullptr || out->rsys == nullptr || wat == nullptr) {
        delete[] wat;
        delete[] out->rsys;
        delete[] out->rdep;
        delete out;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        out->rdep[i] = 0;
        out->rsys[i] = RIVER_LINE_SYS_NONE;
    }
    std::vector<u32> mouths;
    flood_all_water(terrain, riv, w, h, wat, &mouths);
    delete[] wat;
    std::vector<RiverSysEntry> tmp;
    tmp.reserve(mouths.size());
    for (u32 mi = 0; mi < mouths.size(); ++mi) {
        const u32 i = mouths[mi];
        if (out->rdep[i] != 0) {
            continue;
        }
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        const u16 basin_id = mouth_basin(basin, riv, w, h, px, py);
        if (basin_id == static_cast<u16>(RIVER_BASIN_NONE)) {
            continue;
        }
        RiverSysEntry ent = {};
        const u16 sys_ix = static_cast<u16>(tmp.size());
        trace_sys(w, h, riv, basin, px, py, basin_id, sys_ix, out->rdep, out->rsys, &ent);
        if (ent.tile_n == 0) {
            continue;
        }
        tmp.push_back(ent);
    }
    out->sys_n = static_cast<u16>(tmp.size());
    if (out->sys_n == 0) {
        out->sys = nullptr;
        return out;
    }
    out->sys = new RiverSysEntry[out->sys_n];
    if (out->sys == nullptr) {
        delete[] out->rsys;
        delete[] out->rdep;
        delete out;
        return nullptr;
    }
    for (u16 i = 0; i < out->sys_n; ++i) {
        out->sys[i] = tmp[i];
    }
    sort_sys(w, h, out->sys, out->rsys, out->sys_n);
    return out;
}

void Generate_RiverLineData::free_result (RiverLineDataResult* res) {
    if (res == nullptr) {
        return;
    }
    delete[] res->sys;
    delete[] res->rdep;
    delete[] res->rsys;
    delete res;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
