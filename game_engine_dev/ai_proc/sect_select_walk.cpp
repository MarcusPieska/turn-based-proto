//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sect_select_walk.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u32 ssw_rng (u32* s) {
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

static u32 ssw_man (u16 ax, u16 ay, u16 bx, u16 by) {
    const u32 dx = ax > bx ? static_cast<u32>(ax - bx) : static_cast<u32>(bx - ax);
    const u32 dy = ay > by ? static_cast<u32>(ay - by) : static_cast<u32>(by - ay);
    return dx + dy;
}

//================================================================================================================================
//=> - SectorSelect_Walk -
//================================================================================================================================

bool SectorSelect_Walk::begin (
    ExpansionState& st,
    const LandSectorAdj& adj,
    const LandSectorSeedPt* pts,
    u16 pt_n,
    u32 rng) {
    if (!adj.ok() || pts == nullptr || pt_n < adj.sec_n()) {
        return false;
    }
    const u16 sec_n = adj.sec_n();
    if (!st.begin(sec_n)) {
        return false;
    }
    st.rs() = (rng == 0u) ? 1u : rng;
    u16 start = U16_KEY_NULL;
    for (u16 t = 0; t < sec_n; ++t) {
        const u16 s = static_cast<u16>(ssw_rng(&st.rs()) % static_cast<u32>(sec_n));
        if (adj.hd(s) != U16_KEY_NULL) {
            start = s;
            break;
        }
    }
    if (start == U16_KEY_NULL) {
        for (u16 s = 0; s < sec_n; ++s) {
            if (adj.hd(s) != U16_KEY_NULL) {
                start = s;
                break;
            }
        }
    }
    if (start == U16_KEY_NULL) {
        start = static_cast<u16>(ssw_rng(&st.rs()) % static_cast<u32>(sec_n));
    }
    st.set_start(start);
    st.mark(start);
    if (!st.path_push(start)) {
        st.clr();
        return false;
    }
    st.set_origin(pts[start].m_x, pts[start].m_y);
    st.set_first(true);
    st.set_have(true);
    return true;
}

bool SectorSelect_Walk::step (
    ExpansionState& st,
    const LandSectorAdj& adj,
    const LandSectorSeedPt* pts,
    u16* out_sector) {
    if (out_sector == nullptr || !st.ok() || !adj.ok() || pts == nullptr) {
        return false;
    }
    u16 cand[256];
    u16 cn = 0u;
    for (u16 e = adj.hd(st.cur()); e != U16_KEY_NULL; e = adj.nx(e)) {
        const u16 n = adj.to(e);
        if (st.visited(n)) {
            continue;
        }
        if (cn < 256u) {
            cand[cn] = n;
            cn = static_cast<u16>(cn + 1u);
        }
    }
    while (cn == 0u && st.path_pop()) {
        st.set_first(false);
        cn = 0u;
        for (u16 e = adj.hd(st.cur()); e != U16_KEY_NULL; e = adj.nx(e)) {
            const u16 n = adj.to(e);
            if (st.visited(n)) {
                continue;
            }
            if (cn < 256u) {
                cand[cn] = n;
                cn = static_cast<u16>(cn + 1u);
            }
        }
    }
    if (cn == 0u || st.path_n() >= st.sec_n()) {
        return false;
    }
    u16 pick = U16_KEY_NULL;
    if (st.first()) {
        pick = cand[ssw_rng(&st.rs()) % static_cast<u32>(cn)];
        st.set_first(false);
    } else {
        u32 best_d = 0xFFFFFFFFu;
        for (u16 i = 0; i < cn; ++i) {
            const u16 n = cand[i];
            const u32 d = ssw_man(st.ox(), st.oy(), pts[n].m_x, pts[n].m_y);
            if (d < best_d || (d == best_d && (pick == U16_KEY_NULL || n < pick))) {
                best_d = d;
                pick = n;
            }
        }
    }
    if (pick == U16_KEY_NULL) {
        return false;
    }
    st.mark(pick);
    if (!st.path_push(pick)) {
        return false;
    }
    *out_sector = pick;
    return true;
}

bool SectorSelect_Walk::select (
    ExpansionState& st,
    const LandSectorAdj& adj,
    const LandSectorSeedPt* pts,
    u16* out_sector) {
    if (out_sector == nullptr || !st.ok() || !adj.ok()) {
        return false;
    }
    if (st.have()) {
        st.set_have(false);
        *out_sector = st.start();
        return true;
    }
    return step(st, adj, pts, out_sector);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
