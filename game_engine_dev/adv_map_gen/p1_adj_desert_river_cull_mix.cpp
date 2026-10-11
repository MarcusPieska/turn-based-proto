//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_adj_desert_river_cull_mix.h"

#include "p1_gen_desert_river_cull.h"
#include "p1_wb_util.h"
#include "profile_time.h"
#include "wb_que_xy.h"

#include <cstring>

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static const i32 k_dx8[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
static const i32 k_dy8[8] = {-1, -1, -1, 0, 0, 1, 1, 1};

struct SysStat {
    u32 m_n; // River tiles in system
    u32 m_hot; // Hot-desert river tiles in system
};

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static u32 label_systems (
    const u8* riv,
    const P1_TileFlags* flags,
    u16 w,
    u16 h,
    u16* lab,
    SysStat* stats,
    u32 stats_cap) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memset(lab, 0, static_cast<size_t>(n) * sizeof(u16));
    WB_QueXY q;
    if (!q.ok()) {
        return 0;
    }
    u32 sys_n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = tidx(w, x, y);
            if (riv[i] == 0 || lab[i] != 0) {
                continue;
            }
            if (sys_n + 1u >= stats_cap) {
                return 0;
            }
            ++sys_n;
            const u16 id = static_cast<u16>(sys_n);
            stats[id].m_n = 0;
            stats[id].m_hot = 0;
            q.clear();
            lab[i] = id;
            if (!q.push(x, y)) {
                return 0;
            }
            while (q.count() > 0) {
                const u16 px = q.x_at(0);
                const u16 py = q.y_at(0);
                q.drop(1);
                const u32 ci = tidx(w, px, py);
                stats[id].m_n++;
                if (flags[ci].hot_desert != 0u) {
                    stats[id].m_hot++;
                }
                for (i32 k = 0; k < 8; ++k) {
                    const i32 nx = static_cast<i32>(px) + k_dx8[k];
                    const i32 ny = static_cast<i32>(py) + k_dy8[k];
                    if (!in_map(w, h, nx, ny)) {
                        continue;
                    }
                    const u16 ux = static_cast<u16>(nx);
                    const u16 uy = static_cast<u16>(ny);
                    const u32 ni = tidx(w, ux, uy);
                    if (riv[ni] == 0 || lab[ni] != 0) {
                        continue;
                    }
                    lab[ni] = id;
                    if (!q.push(ux, uy)) {
                        return 0;
                    }
                }
            }
        }
    }
    return sys_n;
}

static bool hot_frac_gt (const SysStat& a, const SysStat& b) {
    const u64 ka = static_cast<u64>(a.m_hot) * static_cast<u64>(b.m_n);
    const u64 kb = static_cast<u64>(b.m_hot) * static_cast<u64>(a.m_n);
    if (ka != kb) {
        return ka > kb;
    }
    return a.m_n > b.m_n;
}

static void sort_ids_desc (u16* ids, const SysStat* stats, u32 n) {
    for (u32 i = 1; i < n; ++i) {
        const u16 key = ids[i];
        u32 j = i;
        while (j > 0u && hot_frac_gt(stats[key], stats[ids[j - 1u]])) {
            ids[j] = ids[j - 1u];
            --j;
        }
        ids[j] = key;
    }
}

static u32 count_nz (const u8* a, u32 n) {
    u32 c = 0;
    for (u32 i = 0; i < n; ++i) {
        if (a[i] != 0) {
            c++;
        }
    }
    return c;
}

static bool apply_mix (
    const P1_RunPrm& prm,
    u8* riv,
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* climate,
    const P1_TileFlags* flags,
    u8 harsh_pct,
    P1_Adj_DesertRiverCullMixRslt* out) 
{
    if (out == nullptr || riv == nullptr || terrain == nullptr || climate == nullptr
        || flags == nullptr || !p1_map_size_ok(w, h)) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    Whiteboard_2B wb_lab("P1_Adj_DesertRiverCullMix", "lab", prm.m_seed);
    Whiteboard_8B wb_stats("P1_Adj_DesertRiverCullMix", "stats", prm.m_seed);
    Whiteboard_2B wb_ids("P1_Adj_DesertRiverCullMix", "ids", prm.m_seed);
    Whiteboard_1B wb_sel("P1_Adj_DesertRiverCullMix", "sel", prm.m_seed);
    Whiteboard_1B wb_mild("P1_Adj_DesertRiverCullMix", "mild", prm.m_seed);
    Whiteboard_1B wb_mouth("P1_Adj_DesertRiverCullMix", "mouth", prm.m_seed);
    P1_WB_CHK(wb_lab);
    P1_WB_CHK(wb_stats);
    P1_WB_CHK(wb_ids);
    P1_WB_CHK(wb_sel);
    P1_WB_CHK(wb_mild);
    P1_WB_CHK(wb_mouth);
    u16* lab = wb_lab.get_iter_ptr();
    SysStat* stats = reinterpret_cast<SysStat*>(wb_stats.get_iter_ptr());
    u16* ids = wb_ids.get_iter_ptr();
    u8* sel = wb_sel.raw();
    u8* mild = wb_mild.raw();
    u8* mouth = wb_mouth.raw();
    const u32 stats_cap = WhiteboardMng::tile_n();
    const u32 sys_n = label_systems(riv, flags, w, h, lab, stats, stats_cap);
    if (sys_n == 0u) {
        if (!out->m_harsh.resize(w, h) || !out->m_mild.resize(w, h)) {
            return false;
        }
        std::memset(out->m_harsh.data_w(), 0, static_cast<size_t>(n));
        std::memset(out->m_mild.data_w(), 0, static_cast<size_t>(n));
        out->m_w = w;
        out->m_h = h;
        out->m_sys_n = 0;
        out->m_harsh_n = 0;
        out->m_mild_n = 0;
        out->m_harsh_tiles = 0;
        out->m_mild_tiles = 0;
        out->m_culled_n = 0;
        return true;
    }
    for (u32 i = 0; i < sys_n; ++i) {
        ids[i] = static_cast<u16>(i + 1u);
    }
    sort_ids_desc(ids, stats, sys_n);
    u8 pct = harsh_pct;
    if (pct == 0u) {
        pct = 20u;
    }
    if (pct > 100u) {
        pct = 100u;
    }
    u32 harsh_n = (sys_n * static_cast<u32>(pct) + 99u) / 100u;
    if (harsh_n == 0u) {
        harsh_n = 1u;
    }
    if (harsh_n > sys_n) {
        harsh_n = sys_n;
    }
    std::memset(sel, 0, static_cast<size_t>(sys_n + 1u));
    for (u32 i = 0; i < harsh_n; ++i) {
        sel[ids[i]] = 1;
    }
    if (!out->m_harsh.resize(w, h) || !out->m_mild.resize(w, h)) {
        return false;
    }
    u8* harsh_ov = out->m_harsh.data_w();
    u8* mild_ov = out->m_mild.data_w();
    std::memset(harsh_ov, 0, static_cast<size_t>(n));
    std::memset(mild_ov, 0, static_cast<size_t>(n));
    std::memset(mild, 0, static_cast<size_t>(n));
    std::memset(mouth, 0, static_cast<size_t>(n));
    u32 harsh_tiles = 0;
    u32 mild_tiles = 0;
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || lab[i] == 0) {
            continue;
        }
        if (sel[lab[i]] != 0) {
            harsh_ov[i] = 1;
            mouth[i] = 1;
            harsh_tiles++;
        } else {
            mild_ov[i] = 1;
            mild[i] = 1;
            mild_tiles++;
        }
    }
    const u32 before = count_nz(riv, n);
    P1_Gen_DesertRiverCull cull_mouth(prm);
    P1_Gen_DesertRiverCull cull_tip(prm);
    if (!cull_mouth.generate(mouth, w, h, terrain, climate, false) || !cull_mouth.is_valid()) {
        return false;
    }
    if (!cull_tip.generate(mild, w, h, terrain, climate, true) || !cull_tip.is_valid()) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        riv[i] = (mouth[i] != 0 || mild[i] != 0) ? 1u : 0u;
    }
    out->m_w = w;
    out->m_h = h;
    out->m_sys_n = sys_n;
    out->m_harsh_n = harsh_n;
    out->m_mild_n = sys_n - harsh_n;
    out->m_harsh_tiles = harsh_tiles;
    out->m_mild_tiles = mild_tiles;
    out->m_culled_n = before - count_nz(riv, n);
    return true;
}

//================================================================================================================================
//=> - P1_Adj_DesertRiverCullMix -
//================================================================================================================================

P1_Adj_DesertRiverCullMix::P1_Adj_DesertRiverCullMix (
    const P1_RunPrm& prm,
    const P1_Adj_DesertRiverCullMixPrm& sp) :
    m_prm(prm),
    m_sp(sp),
    m_ok(false),
    m_rslt() {
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sys_n = 0;
    m_rslt.m_harsh_n = 0;
    m_rslt.m_mild_n = 0;
    m_rslt.m_harsh_tiles = 0;
    m_rslt.m_mild_tiles = 0;
    m_rslt.m_culled_n = 0;
}

bool P1_Adj_DesertRiverCullMix::adjust (
    u8* riv,
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* climate,
    const P1_TileFlags* flags) 
{
    PTIME_START(__func__);
    m_ok = false;
    m_rslt.m_harsh.clear();
    m_rslt.m_mild.clear();
    m_rslt.m_w = 0;
    m_rslt.m_h = 0;
    m_rslt.m_sys_n = 0;
    m_rslt.m_harsh_n = 0;
    m_rslt.m_mild_n = 0;
    m_rslt.m_harsh_tiles = 0;
    m_rslt.m_mild_tiles = 0;
    m_rslt.m_culled_n = 0;
    if (riv == nullptr || terrain == nullptr || climate == nullptr || flags == nullptr
        || w != m_prm.m_w || h != m_prm.m_h || !p1_run_prm_ok(m_prm)) {
        PTIME_STOP(__func__);
        return false;
    }
    if (!apply_mix(m_prm, riv, w, h, terrain, climate, flags, m_sp.m_harsh_pct, &m_rslt)) {
        m_rslt.m_harsh.clear();
        m_rslt.m_mild.clear();
        PTIME_STOP(__func__);
        return false;
    }
    m_ok = true;
    PTIME_STOP(__func__);
    return true;
}

bool P1_Adj_DesertRiverCullMix::is_valid () const {
    return m_ok;
}

const P1_Adj_DesertRiverCullMixRslt& P1_Adj_DesertRiverCullMix::result () const {
    return m_rslt;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
