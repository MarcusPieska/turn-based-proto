//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sect_select_integral.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u32 ssi_rng (u32* s) {
    *s = (*s * 1664525u) + 1013904223u;
    return *s;
}

//================================================================================================================================
//=> - SectorSelect_Integral -
//================================================================================================================================

bool SectorSelect_Integral::begin (
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
        const u16 s = static_cast<u16>(ssi_rng(&st.rs()) % static_cast<u32>(sec_n));
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
        start = static_cast<u16>(ssi_rng(&st.rs()) % static_cast<u32>(sec_n));
    }
    st.set_start(start);
    st.mark(start);
    if (!st.path_push(start)) {
        st.clr();
        return false;
    }
    st.set_first(true);
    st.set_have(true);
    return true;
}

i32 SectorSelect_Integral::score (ExpansionState& st, const LandSectorAdj& adj, u16 n) {
    i32 sc = 0;
    for (u16 e = adj.hd(n); e != U16_KEY_NULL; e = adj.nx(e)) {
        const u16 o = adj.to(e);
        if (st.visited(o)) {
            ++sc;
        } else {
            --sc;
        }
    }
    return sc;
}

bool SectorSelect_Integral::step (
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
        pick = cand[ssi_rng(&st.rs()) % static_cast<u32>(cn)];
        st.set_first(false);
    } else {
        i32 best_sc = 0;
        u32 best_yld = 0u;
        for (u16 i = 0; i < cn; ++i) {
            const u16 n = cand[i];
            const i32 sc = score(st, adj, n);
            const u32 yld = pts[n].m_yields;
            if (pick == U16_KEY_NULL
                || sc > best_sc
                || (sc == best_sc && yld > best_yld)
                || (sc == best_sc && yld == best_yld && n < pick)) {
                pick = n;
                best_sc = sc;
                best_yld = yld;
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

bool SectorSelect_Integral::select (
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
