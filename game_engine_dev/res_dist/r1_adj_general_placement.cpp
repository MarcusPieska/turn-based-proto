//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include "r1_adj_general_placement.h"

#include "r1_adj_res_place.h"
#include "res_dist_static_key.h"
#include "resource_static_key.h"

//================================================================================================================================
//= - R1_Adj_GeneralPlacement -
//================================================================================================================================

#define R1_GEN_FAIR_CAP 128

R1_Adj_GeneralPlacement::R1_Adj_GeneralPlacement ()
    : m_ok(false)
    , m_plc_n(0)
{
}

bool R1_Adj_GeneralPlacement::adjust (
    u16* res_ov,
    u16 w,
    u16 h,
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    ResDistState& st,
    u32 base_n,
    u32 seed)
{
    m_ok = false;
    m_plc_n = 0;
    if (res_ov == nullptr || w == 0 || h == 0 || ctx.m_terrain == nullptr || base_n == 0) {
        return false;
    }
    if (ctx.m_w != w || ctx.m_h != h) {
        return false;
    }
    const u16 res_n = s.resource().get_item_count();
    if (st.res_n() == 0) {
        if (!st.reset(res_n)) {
            return false;
        }
    } else if (st.res_n() != res_n) {
        return false;
    }
    u16 ids[R1_GEN_FAIR_CAP];
    u32 soft[R1_GEN_FAIR_CAP];
    u32 hard[R1_GEN_FAIR_CAP];
    u32 got[R1_GEN_FAIR_CAP];
    u16 n = 0;
    for (u16 ri = 0; ri < res_n; ++ri) {
        const ResourceStaticDataStruct& r = s.resource().get_item(ResourceStaticDataKey::from_raw(ri));
        if (r.res_dist_idx >= s.res_dist().get_item_count()) {
            continue;
        }
        const ResDistStaticDataStruct& rd = s.res_dist().get_item(ResDistStaticDataKey::from_raw(r.res_dist_idx));
        if (rd.has_plc == 0) {
            continue;
        }
        const u32 target = base_n * static_cast<u32>(rd.plc.m_res_wt);
        if (target == 0 || st.met(ri, target)) {
            continue;
        }
        if (n >= R1_GEN_FAIR_CAP) {
            return false;
        }
        const u32 cur = st.get(ri);
        const u32 cap = target * 2u;
        ids[n] = ri;
        soft[n] = target - cur;
        hard[n] = cur < cap ? (cap - cur) : 0u;
        if (hard[n] < soft[n]) {
            hard[n] = soft[n];
        }
        got[n] = 0;
        n = static_cast<u16>(n + 1u);
    }
    if (n == 0) {
        m_ok = true;
        return true;
    }
    u32 tot = 0;
    if (!r1_adj_res_place_fair(res_ov, w, h, ctx, s, ids, soft, hard, n, seed, got, &tot)) {
        return false;
    }
    for (u16 i = 0; i < n; ++i) {
        if (got[i] != 0) {
            st.add(ids[i], got[i]);
        }
    }
    m_plc_n = tot;
    m_ok = true;
    return true;
}

bool R1_Adj_GeneralPlacement::is_valid () const {
    return m_ok;
}

u32 R1_Adj_GeneralPlacement::placed_n () const {
    return m_plc_n;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
