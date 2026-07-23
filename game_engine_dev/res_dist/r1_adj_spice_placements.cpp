//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include "r1_adj_spice_placements.h"

#include <cstdio>
#include <cstring>

#include "game_map_defs.h"
#include "p1_wb_util.h"
#include "res_dist_static_key.h"
#include "res_type_static_key.h"
#include "resource_static_key.h"
#include "wb_que_xy.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//= - Helpers -
//================================================================================================================================

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

struct SpcRng {
    u32 m_s; // LCG state
};

static u32 spc_rng_next (SpcRng* r) {
    r->m_s = r->m_s * 1103515245u + 12345u;
    return (r->m_s >> 16u) & 0x7fffu;
}


static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
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

static u16 find_spc_type (const RuntimeStatics& s) {
    const ResTypeStaticData& rt = s.res_type();
    const u16 n = rt.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = rt.get_name(ResTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "SPICE") == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static const ResDistStaticDataStruct* res_rd (const RuntimeStatics& s, u16 res_i) {
    const ResourceStaticDataStruct& r = s.resource().get_item(ResourceStaticDataKey::from_raw(res_i));
    if (r.res_dist_idx >= s.res_dist().get_item_count()) {
        return nullptr;
    }
    return &s.res_dist().get_item(ResDistStaticDataKey::from_raw(r.res_dist_idx));
}

static bool res_tile_ok (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u32 idx)
{
    const ResDistStaticDataStruct* rd = res_rd(s, res_i);
    if (rd == nullptr || rd->has_plc == 0) {
        return false;
    }
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        if (ResPlcMatch::entry_ok(ctx, idx, s, res_i, qi)) {
            return true;
        }
    }
    return false;
}

static bool flood_sec_pool (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    const u16* sec_ov,
    u16 w,
    u16 h,
    u16 sid,
    u16 sx,
    u16 sy,
    u16 res_i,
    u16* vis,
    u16 stamp,
    WB_QueXY& que,
    u32* pool,
    u32 pool_cap,
    u32* pool_n)
{
    if (pool == nullptr || pool_n == nullptr || sec_ov == nullptr || vis == nullptr) {
        return false;
    }
    const u32 wi = static_cast<u32>(w);
    const u32 hi = static_cast<u32>(h);
    const u32 npx = wi * hi;
    que.clear();
    const u32 s0 = static_cast<u32>(sy) * wi + static_cast<u32>(sx);
    if (s0 >= npx || sec_ov[s0] != sid) {
        return true;
    }
    if (!que.push(sx, sy)) {
        return false;
    }
    vis[s0] = stamp;
    u32 qi = 0;
    while (qi < que.count()) {
        const u16 px = que.x_at(qi);
        const u16 py = que.y_at(qi);
        qi = qi + 1u;
        const u32 ti = static_cast<u32>(py) * wi + static_cast<u32>(px);
        if (res_tile_ok(ctx, s, res_i, ti) && *pool_n < pool_cap) {
            pool[*pool_n] = ti;
            *pool_n = *pool_n + 1u;
        }
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(px) + k_dx4[d];
            const i32 ny = static_cast<i32>(py) + k_dy4[d];
            if (nx < 0 || ny < 0 || static_cast<u32>(nx) >= wi || static_cast<u32>(ny) >= hi) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * wi + static_cast<u32>(nx);
            if (vis[ni] == stamp || sec_ov[ni] != sid) {
                continue;
            }
            vis[ni] = stamp;
            if (!que.push(static_cast<u16>(nx), static_cast<u16>(ny))) {
                return false;
            }
        }
    }
    return true;
}

//================================================================================================================================
//= - R1_Adj_SpicePlacements -
//================================================================================================================================

R1_Adj_SpicePlacements::R1_Adj_SpicePlacements ()
    : m_ok(false)
    , m_w(0)
    , m_h(0)
    , m_spc_n(0)
    , m_sec_n(0)
    , m_asg_n(0)
    , m_plc_n(0)
{
}

R1_Adj_SpicePlacements::~R1_Adj_SpicePlacements () {
    clr();
}

void R1_Adj_SpicePlacements::clr () {
    m_ok = false;
    m_w = 0;
    m_h = 0;
    m_spc_n = 0;
    m_sec_n = 0;
    m_asg_n = 0;
    m_plc_n = 0;
}

bool R1_Adj_SpicePlacements::adjust (
    u16* res_ov,
    const RuntimeStatics& s,
    ResDistState& st,
    const ResPlcMapCtx& ctx,
    const u16* sec_ov,
    u16 sec_n,
    u32 base_n,
    u32 seed)
{
    clr();
    if (res_ov == nullptr || sec_ov == nullptr || ctx.m_terrain == nullptr || ctx.m_w == 0 || ctx.m_h == 0
        || sec_n == 0 || base_n == 0) {
        return false;
    }
    m_w = ctx.m_w;
    m_h = ctx.m_h;
    const u16 spc_ty = find_spc_type(s);
    if (spc_ty == U16_KEY_NULL) {
        std::printf("SPICE res_type not found\n");
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
    const ResourceStaticData& rs = s.resource();
    for (u16 i = 0; i < res_n; ++i) {
        const ResourceStaticDataStruct& it = rs.get_item(ResourceStaticDataKey::from_raw(i));
        if (it.type != spc_ty) {
            continue;
        }
        if (m_spc_n < R1_SPC_PLC_CAP) {
            m_spc[m_spc_n] = i;
            m_spc_n = static_cast<u16>(m_spc_n + 1u);
        }
    }
    if (m_spc_n == 0) {
        std::printf("no SPICE resources found\n");
        return false;
    }
    const u32 npx = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    const u16 sec_cap = (sec_n < R1_SPC_PLC_CAP) ? sec_n : static_cast<u16>(R1_SPC_PLC_CAP);
    R1_SpcSecRec slots[R1_SPC_PLC_CAP];
    u8 have[R1_SPC_PLC_CAP];
    for (u16 i = 0; i < sec_cap; ++i) {
        have[i] = 0;
    }
    for (u32 i = 0; i < npx; ++i) {
        const u16 sid = sec_ov[i];
        if (sid >= sec_cap || have[sid] != 0) {
            continue;
        }
        have[sid] = 1;
        slots[sid].m_x = static_cast<u16>(i % static_cast<u32>(m_w));
        slots[sid].m_y = static_cast<u16>(i / static_cast<u32>(m_w));
        slots[sid].m_sec = sid;
    }
    for (u16 sid = 0; sid < sec_cap; ++sid) {
        if (have[sid] == 0) {
            std::printf("sector %u has no tiles\n", (unsigned)sid);
            continue;
        }
        m_sec[m_sec_n] = slots[sid];
        m_sec_n = static_cast<u16>(m_sec_n + 1u);
    }
    if (m_sec_n == 0) {
        std::printf("no sector start points\n");
        return false;
    }
    SpcRng cull;
    cull.m_s = seed == 0u ? 1u : (seed ^ 0xA5A5A5A5u);
    u16 keep_n = static_cast<u16>((static_cast<u32>(m_sec_n) * static_cast<u32>(R1_SPC_SEC_KEEP_PCT)) / 100u);
    if (keep_n == 0) {
        keep_n = 1;
    }
    if (keep_n > m_sec_n) {
        keep_n = m_sec_n;
    }
    for (u16 i = 0; i < keep_n; ++i) {
        const u16 j = static_cast<u16>(i + (spc_rng_next(&cull) % static_cast<u32>(m_sec_n - i)));
        const R1_SpcSecRec tmp = m_sec[i];
        m_sec[i] = m_sec[j];
        m_sec[j] = tmp;
    }
    m_sec_n = keep_n;
    if (m_sec_n >= m_spc_n) {
        for (u16 i = 0; i < m_sec_n; ++i) {
            m_asg[m_asg_n].m_res = m_spc[static_cast<u16>(i % m_spc_n)];
            m_asg[m_asg_n].m_sec = m_sec[i].m_sec;
            m_asg[m_asg_n].m_x = m_sec[i].m_x;
            m_asg[m_asg_n].m_y = m_sec[i].m_y;
            m_asg_n = static_cast<u16>(m_asg_n + 1u);
        }
    } else {
        for (u16 i = 0; i < m_spc_n; ++i) {
            const u16 si = static_cast<u16>(i % m_sec_n);
            m_asg[m_asg_n].m_res = m_spc[i];
            m_asg[m_asg_n].m_sec = m_sec[si].m_sec;
            m_asg[m_asg_n].m_x = m_sec[si].m_x;
            m_asg[m_asg_n].m_y = m_sec[si].m_y;
            m_asg_n = static_cast<u16>(m_asg_n + 1u);
        }
    }
    Whiteboard_2B wb_vis("R1_Adj_SpicePlacements", "vis", 0u);
    P1_WB_CHK(wb_vis);
    u16* vis = wb_vis.get_iter_ptr();
    for (u32 i = 0; i < npx; ++i) {
        vis[i] = 0;
    }
    WB_QueXY que;
    if (!que.ok()) {
        return false;
    }
    Whiteboard_4B wb_pool("R1_Adj_SpicePlacements", "pool", 0u);
    P1_WB_CHK(wb_pool);
    u32* pool = wb_pool.get_iter_ptr();
    SpcRng rng;
    rng.m_s = seed == 0u ? 1u : seed;
    u16 stamp = 0;
    for (u16 gi = 0; gi < m_spc_n; ++gi) {
        const u16 res_i = m_spc[gi];
        const ResDistStaticDataStruct* rd = res_rd(s, res_i);
        if (rd == nullptr || rd->has_plc == 0) {
            continue;
        }
        const u32 total = base_n * static_cast<u32>(rd->plc.m_res_wt);
        if (total == 0 || st.met(res_i, total)) {
            continue;
        }
        u32 pool_n = 0;
        for (u16 ai = 0; ai < m_asg_n; ++ai) {
            if (m_asg[ai].m_res != res_i) {
                continue;
            }
            stamp = static_cast<u16>(stamp + 1u);
            if (stamp == 0) {
                for (u32 i = 0; i < npx; ++i) {
                    vis[i] = 0;
                }
                stamp = 1;
            }
            if (!flood_sec_pool(ctx, s, sec_ov, m_w, m_h, m_asg[ai].m_sec, m_asg[ai].m_x, m_asg[ai].m_y,
                    res_i, vis, stamp, que, pool, npx, &pool_n)) {
                return false;
            }
        }
        u32 rem = pool_n;
        u32 left = total;
        u32 placed = 0;
        while (left > 0 && rem > 0) {
            const u32 pick = spc_rng_next(&rng) % rem;
            const u32 idx = pool[pick];
            pool[pick] = pool[rem - 1u];
            rem = rem - 1u;
            if (res_ov[idx] != U16_KEY_NULL) {
                continue;
            }
            res_ov[idx] = res_i;
            placed = placed + 1u;
            m_plc_n = m_plc_n + 1u;
            left = left - 1u;
        }
        st.add(res_i, placed);
    }
    m_ok = true;
    return true;
}

bool R1_Adj_SpicePlacements::is_valid () const {
    return m_ok;
}

u16 R1_Adj_SpicePlacements::spc_n () const {
    return m_spc_n;
}

u16 R1_Adj_SpicePlacements::spc_at (u16 i) const {
    return m_spc[i];
}

u16 R1_Adj_SpicePlacements::sec_n () const {
    return m_sec_n;
}

u16 R1_Adj_SpicePlacements::asg_n () const {
    return m_asg_n;
}

const R1_SpcAsgRec& R1_Adj_SpicePlacements::asg_at (u16 i) const {
    return m_asg[i];
}

u32 R1_Adj_SpicePlacements::plc_n () const {
    return m_plc_n;
}

bool R1_Adj_SpicePlacements::save_res_ppm (
    cstr path,
    const u8* terr,
    const u16* res_ov,
    u16 res_i) const
{
    if (!m_ok || path == nullptr || terr == nullptr || res_ov == nullptr || m_w == 0 || m_h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(m_w) * static_cast<u32>(m_h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        terr_rgb(terr[i], &r, &g, &b);
        if (res_ov[i] == res_i) {
            r = 255;
            g = 0;
            b = 0;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)m_w, (unsigned)m_h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
