//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "p1_gen_watershed_mountain_line_sets.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private generation helpers -
//================================================================================================================================

static u32 tile_cap_n (u32 border_tile_n, u8 perc) {
    if (border_tile_n == 0 || perc == 0) {
        return 0;
    }
    if (perc >= 100) {
        return border_tile_n;
    }
    return (border_tile_n * static_cast<u32>(perc) + 99u) / 100u;
}

static u32 max_mouth_dist (const P1_WatershedBorderSeg* segs, u16 seg_n) {
    if (segs == nullptr || seg_n == 0) {
        return 0;
    }
    return segs[seg_n - 1].m_mouth_dist;
}

static bool seg_dist_too_short (u32 mouth_dist, u32 max_dist, u8 min_perc) {
    if (max_dist == 0 || min_perc == 0) {
        return false;
    }
    if (min_perc >= 100) {
        return false;
    }
    return mouth_dist * 100u < max_dist * static_cast<u32>(min_perc);
}

static bool pick_line_sets (
    const P1_Gen_WatershedMountainsRslt& borders,
    const P1_Gen_WatershedMountainLineSetsPrm& sp,
    P1_Gen_WatershedMountainLineSetsRslt* out) 
{
    if (out == nullptr || borders.m_ov == nullptr || borders.m_segs == nullptr) {
        return false;
    }
    const u16 w = borders.m_w;
    const u16 h = borders.m_h;
    const u16 seg_n = borders.m_seg_n;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    out->m_w = w;
    out->m_h = h;
    out->m_pick_n = 0;
    out->m_pick_tile_n = 0;
    out->m_pick_seg = nullptr;
    out->m_ov = new u16[n];
    if (out->m_ov == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        out->m_ov[i] = 0;
    }
    if (seg_n == 0) {
        return true;
    }
    const u32 tile_cap = tile_cap_n(borders.m_border_tile_n, sp.m_tile_cap_perc);
    const u32 max_dist = max_mouth_dist(borders.m_segs, seg_n);
    u32 covered = 0;
    u16 pick_n = 0;
    u16* pick_seg = new u16[seg_n];
    if (pick_seg == nullptr) {
        delete[] out->m_ov;
        out->m_ov = nullptr;
        return false;
    }
    for (u16 sj = seg_n; sj > 0; --sj) {
        if (pick_n >= sp.m_max_pick_n) {
            break;
        }
        if (covered >= tile_cap) {
            break;
        }
        const u16 idx = static_cast<u16>(sj - 1u);
        const P1_WatershedBorderSeg& s = borders.m_segs[idx];
        if (seg_dist_too_short(s.m_mouth_dist, max_dist, sp.m_min_dist_perc)) {
            break;
        }
        if (s.m_tile_n == 0) {
            continue;
        }
        if (covered + s.m_tile_n > tile_cap) {
            break;
        }
        for (u32 i = 0; i < n; ++i) {
            if (borders.m_ov[i] == s.m_ov_idx) {
                out->m_ov[i] = s.m_ov_idx;
            }
        }
        pick_seg[pick_n] = idx;
        covered += s.m_tile_n;
        pick_n++;
    }
    if (pick_n == 0) {
        delete[] pick_seg;
        return true;
    }
    u16* pick_trim = new u16[pick_n];
    if (pick_trim == nullptr) {
        delete[] pick_seg;
        return false;
    }
    for (u16 i = 0; i < pick_n; ++i) {
        pick_trim[i] = pick_seg[i];
    }
    delete[] pick_seg;
    out->m_pick_n = pick_n;
    out->m_pick_tile_n = covered;
    out->m_pick_seg = pick_trim;
    return true;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 32;
    *g = 32;
    *b = 32;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
}

static u16 max_basin_idx (const P1_Gen_RiverNetworkRslt& network) {
    u16 mx = 0;
    for (u16 bi = 0; bi < network.m_basin_n; ++bi) {
        if (network.m_basins[bi].m_idx > mx) {
            mx = network.m_basins[bi].m_idx;
        }
    }
    return mx;
}

//================================================================================================================================
//=> - P1_Gen_WatershedMountainLineSets -
//================================================================================================================================

P1_Gen_WatershedMountainLineSets::P1_Gen_WatershedMountainLineSets (
    const P1_RunPrm& prm,
    const P1_Gen_WatershedMountainLineSetsPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_valid_generation(false),
    m_rslt() {
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

P1_Gen_WatershedMountainLineSets::~P1_Gen_WatershedMountainLineSets () {
    clear_rslt();
}

void P1_Gen_WatershedMountainLineSets::clear_rslt () {
    delete[] m_rslt.m_pick_seg;
    delete[] m_rslt.m_ov;
    std::memset(&m_rslt, 0, sizeof(m_rslt));
}

bool P1_Gen_WatershedMountainLineSets::generate (const P1_Gen_WatershedMountainsRslt& borders) {
    m_valid_generation = false;
    clear_rslt();
    if (!p1_map_size_ok(borders.m_w, borders.m_h) || borders.m_w != m_prm.m_w || borders.m_h != m_prm.m_h) {
        return false;
    }
    if (!pick_line_sets(borders, m_sp, &m_rslt)) {
        clear_rslt();
        return false;
    }
    m_valid_generation = true;
    return true;
}

bool P1_Gen_WatershedMountainLineSets::is_valid () const {
    return m_valid_generation;
}

const P1_Gen_WatershedMountainLineSetsRslt& P1_Gen_WatershedMountainLineSets::result () const {
    return m_rslt;
}

void P1_Gen_WatershedMountainLineSets::save_output (
    cstr path,
    const P1_Gen_RiverNetworkRslt& network,
    const u8* terrain) const 
{
    if (!m_valid_generation || path == nullptr || network.m_ov == nullptr || m_rslt.m_ov == nullptr) {
        return;
    }
    const u16 w = m_rslt.m_w;
    const u16 h = m_rslt.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 32;
        u8 g = 32;
        u8 b = 32;
        if (terrain != nullptr) {
            terr_rgb(terrain[i], &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const u16 pal_cap = static_cast<u16>(max_basin_idx(network) + 1u);
    u8* pal = new u8[static_cast<size_t>(pal_cap) * 3u];
    bool* pal_set = new bool[pal_cap];
    if (pal == nullptr || pal_set == nullptr) {
        delete[] pal_set;
        delete[] pal;
        delete[] rgb;
        return;
    }
    for (u16 i = 0; i < pal_cap; ++i) {
        pal_set[i] = false;
    }
    Rng32 rng;
    rng_seed(&rng, m_prm.m_seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u16 bid = network.m_ov[i];
        if (bid == static_cast<u16>(P1_RIVER_BASIN_NONE) || bid >= pal_cap) {
            continue;
        }
        if (!pal_set[bid]) {
            pal[bid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal_set[bid] = true;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = pal[bid * 3u + 0];
        rgb[p + 1] = pal[bid * 3u + 1];
        rgb[p + 2] = pal[bid * 3u + 2];
    }
    for (u32 i = 0; i < n; ++i) {
        if (m_rslt.m_ov[i] == 0) {
            continue;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = 0;
        rgb[p + 1] = 0;
        rgb[p + 2] = 0;
    }
    save_rgb_ppm(path, rgb, w, h);
    delete[] pal_set;
    delete[] pal;
    delete[] rgb;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
