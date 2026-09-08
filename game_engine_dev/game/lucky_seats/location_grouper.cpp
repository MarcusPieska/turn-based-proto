//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "location_grouper.h"

//================================================================================================================================
//=> - LocGroups -
//================================================================================================================================

LocGroups::LocGroups () :
    m_n(0u)
{
    for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
        m_g[i].m_pts = nullptr;
        m_g[i].m_sc = nullptr;
        m_g[i].m_n = 0u;
        m_g[i].m_cap = 0u;
        m_g[i].m_rank = 0u;
    }
}

LocGroups::~LocGroups () {
    clear();
}

void LocGroups::clear () {
    for (u16 i = 0; i < ContSizeList::k_cap; ++i) {
        delete[] m_g[i].m_pts;
        delete[] m_g[i].m_sc;
        m_g[i].m_pts = nullptr;
        m_g[i].m_sc = nullptr;
        m_g[i].m_n = 0u;
        m_g[i].m_cap = 0u;
        m_g[i].m_rank = 0u;
    }
    m_n = 0u;
}

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static void sort_desc (LocGroup* g) {
    if (g == nullptr || g->m_n < 2u) {
        return;
    }
    for (u16 a = 0; a < g->m_n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < g->m_n; ++b) {
            if (g->m_sc[b] > g->m_sc[a]) {
                const i32 ts = g->m_sc[a];
                g->m_sc[a] = g->m_sc[b];
                g->m_sc[b] = ts;
                const SpgCoordPair tp = g->m_pts[a];
                g->m_pts[a] = g->m_pts[b];
                g->m_pts[b] = tp;
            }
        }
    }
}

//================================================================================================================================
//=> - LocationGrouper -
//================================================================================================================================

bool LocationGrouper::group (
    const SpgCoordPair* starts,
    const i32* scores,
    u16 n,
    const u16* land_idx,
    u16 w,
    u16 h,
    u16 cont_n,
    LocGroups* out)
{
    if (starts == nullptr || scores == nullptr || land_idx == nullptr || out == nullptr) {
        return false;
    }
    if (n == 0u || w == 0u || h == 0u || cont_n == 0u || cont_n > ContSizeList::k_cap) {
        return false;
    }
    out->clear();
    out->m_n = cont_n;
    for (u16 i = 0; i < cont_n; ++i) {
        LocGroup& g = out->m_g[i];
        g.m_pts = new SpgCoordPair[n];
        g.m_sc = new i32[n];
        if (g.m_pts == nullptr || g.m_sc == nullptr) {
            out->clear();
            return false;
        }
        g.m_n = 0u;
        g.m_cap = n;
        g.m_rank = static_cast<u16>(i + 1u);
    }
    const u32 tn = static_cast<u32>(w) * static_cast<u32>(h);
    for (u16 i = 0; i < n; ++i) {
        const u16 x = starts[i].x;
        const u16 y = starts[i].y;
        if (x >= w || y >= h) {
            continue;
        }
        const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
        if (ti >= tn) {
            continue;
        }
        const u16 rank = land_idx[ti];
        if (rank == 0u || rank > cont_n) {
            continue;
        }
        LocGroup& g = out->m_g[rank - 1u];
        if (g.m_n >= g.m_cap) {
            continue;
        }
        g.m_pts[g.m_n] = starts[i];
        g.m_sc[g.m_n] = scores[i];
        g.m_n = static_cast<u16>(g.m_n + 1u);
    }
    for (u16 i = 0; i < cont_n; ++i) {
        sort_desc(&out->m_g[i]);
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
