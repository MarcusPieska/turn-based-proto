//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include "r1_adj_res_place.h"

#include <ctime>

#include "res_dist_static_key.h"
#include "res_placement.h"
#include "resource_static_key.h"

//================================================================================================================================
//= - Private helpers -
//================================================================================================================================

#define R1_PLC_POOL_MUL 4u
#define R1_PLC_POOL_MIN 64u
#define R1_PLC_FAIR_CAP 128

typedef struct ResPlcPool {
    u32* m_idx;
    u32 m_n;
    u32 m_cap;
    u32 m_seen;
} ResPlcPool;

static const ResDistStaticDataStruct* res_rd (
    const RuntimeStatics& s,
    u16 res_i)
{
    const ResourceStaticDataStruct& r = s.resource().get_item(
        ResourceStaticDataKey::from_raw(res_i));
    if (r.res_dist_idx >= s.res_dist().get_item_count()) {
        return nullptr;
    }
    return &s.res_dist().get_item(ResDistStaticDataKey::from_raw(r.res_dist_idx));
}

static void pool_free (ResPlcPool* p) {
    if (p == nullptr) {
        return;
    }
    delete[] p->m_idx;
    p->m_idx = nullptr;
    p->m_n = 0;
    p->m_cap = 0;
    p->m_seen = 0;
}

static bool pool_init (ResPlcPool* p, u32 cap) {
    if (p == nullptr || cap == 0) {
        return false;
    }
    p->m_idx = new u32[cap];
    if (p->m_idx == nullptr) {
        return false;
    }
    p->m_n = 0;
    p->m_cap = cap;
    p->m_seen = 0;
    return true;
}

static u32 lcg_next (u32* seed) {
    *seed = (*seed * 1103515245u + 12345u);
    return (*seed >> 16u) & 0x7fffu;
}

static void pool_resv (ResPlcPool* p, u32 idx, u32* seed) {
    if (p == nullptr || p->m_idx == nullptr || seed == nullptr) {
        return;
    }
    p->m_seen = p->m_seen + 1u;
    if (p->m_n < p->m_cap) {
        p->m_idx[p->m_n] = idx;
        p->m_n = p->m_n + 1u;
        return;
    }
    const u32 j = lcg_next(seed) % p->m_seen;
    if (j < p->m_cap) {
        p->m_idx[j] = idx;
    }
}

static u32 wt_sum (const ResPlacement& plc) {
    u32 s = 0;
    for (u8 qi = 0; qi < plc.m_quad_n; ++qi) {
        s += (u32)plc.m_quads[qi].m_wt;
    }
    return s;
}

static u32 quota_for (u32 total, u32 wt, u32 wt_tot, u32* rem) {
    if (wt_tot == 0) {
        return 0;
    }
    const u64 num = (u64)total * (u64)wt + (u64)(*rem);
    const u32 q = (u32)(num / (u64)wt_tot);
    *rem = (u32)(num % (u64)wt_tot);
    return q;
}

static bool tile_open_u16 (const u16* res_ov, u32 idx) {
    return res_ov == nullptr || res_ov[idx] == U16_KEY_NULL;
}

static bool tile_open_u8 (const u8* out, u32 idx) {
    return out == nullptr || out[idx] == 0;
}

static u32 pool_cap_for (u32 want) {
    u32 c = want * R1_PLC_POOL_MUL;
    if (c < R1_PLC_POOL_MIN) {
        c = R1_PLC_POOL_MIN;
    }
    if (c < want) {
        c = want;
    }
    return c;
}

static u32 draw_u16 (
    ResPlcPool* pool,
    u32 want,
    u32* seed,
    u16 res_i,
    u16* res_ov)
{
    if (pool == nullptr || res_ov == nullptr || seed == nullptr || want == 0) {
        return 0;
    }
    u32 placed = 0;
    u32 rem = pool->m_n;
    for (u32 d = 0; d < pool->m_n && placed < want && rem > 0; ++d) {
        const u32 pick = lcg_next(seed) % rem;
        const u32 idx = pool->m_idx[pick];
        pool->m_idx[pick] = pool->m_idx[rem - 1u];
        --rem;
        if (!tile_open_u16(res_ov, idx)) {
            continue;
        }
        res_ov[idx] = res_i;
        ++placed;
    }
    pool->m_n = rem;
    return placed;
}

static u32 draw_u8 (
    ResPlcPool* pool,
    u32 want,
    u32* seed,
    u8 rule_idx,
    u8* out)
{
    if (pool == nullptr || out == nullptr || seed == nullptr || want == 0) {
        return 0;
    }
    u32 placed = 0;
    u32 rem = pool->m_n;
    for (u32 d = 0; d < pool->m_n && placed < want && rem > 0; ++d) {
        const u32 pick = lcg_next(seed) % rem;
        const u32 idx = pool->m_idx[pick];
        pool->m_idx[pick] = pool->m_idx[rem - 1u];
        --rem;
        if (!tile_open_u8(out, idx)) {
            continue;
        }
        out[idx] = rule_idx;
        ++placed;
    }
    pool->m_n = rem;
    return placed;
}

static u32 draw_any_u16 (
    ResPlcPool* pools,
    u8 quad_n,
    u32 want,
    u32* seed,
    u16 res_i,
    u16* res_ov)
{
    u32 placed = 0;
    while (placed < want) {
        u32 got = 0;
        for (u8 qi = 0; qi < quad_n && placed < want; ++qi) {
            const u32 d = draw_u16(&pools[qi], 1u, seed, res_i, res_ov);
            placed += d;
            got += d;
        }
        if (got == 0) {
            break;
        }
    }
    return placed;
}

static u32 draw_any_u8 (
    ResPlcPool* pools,
    u8 quad_n,
    u32 want,
    u32* seed,
    u8* out)
{
    u32 placed = 0;
    while (placed < want) {
        u32 got = 0;
        for (u8 qi = 0; qi < quad_n && placed < want; ++qi) {
            const u32 d = draw_u8(&pools[qi], 1u, seed, (u8)(qi + 1u), out);
            placed += d;
            got += d;
        }
        if (got == 0) {
            break;
        }
    }
    return placed;
}

static bool place_core (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u32 base_n,
    u32 seed,
    u16* res_ov,
    u8* u8_out,
    u32 want_n,
    u32* placed_n,
    double* sec_out)
{
    if (placed_n != nullptr) {
        *placed_n = 0;
    }
    if (sec_out != nullptr) {
        *sec_out = 0.0;
    }
    const ResDistStaticDataStruct* rd = res_rd(s, res_i);
    if (rd == nullptr || rd->has_plc == 0) {
        return false;
    }
    if (res_ov == nullptr && u8_out == nullptr) {
        return false;
    }
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (base_n == 0 || n == 0) {
        return false;
    }
    const clock_t t0 = clock();
    if (u8_out != nullptr) {
        for (u32 i = 0; i < n; ++i) {
            u8_out[i] = 0;
        }
    }
    const ResPlacement& plc = rd->plc;
    const u32 wt_tot = wt_sum(plc);
    if (wt_tot == 0 || plc.m_quad_n == 0) {
        return false;
    }
    const u32 soft = want_n != 0u ? want_n : (base_n * (u32)plc.m_res_wt);
    if (soft == 0) {
        return false;
    }
    const u32 hard = soft * 2u;
    ResPlcPool pools[RES_IO_QUAD_MAX];
    for (u8 qi = 0; qi < RES_IO_QUAD_MAX; ++qi) {
        pools[qi].m_idx = nullptr;
        pools[qi].m_n = 0;
        pools[qi].m_cap = 0;
        pools[qi].m_seen = 0;
    }
    u32 rem_q = 0;
    u32 qwant[RES_IO_QUAD_MAX];
    for (u8 qi = 0; qi < plc.m_quad_n; ++qi) {
        qwant[qi] = quota_for(soft, (u32)plc.m_quads[qi].m_wt, wt_tot, &rem_q);
        if (!pool_init(&pools[qi], pool_cap_for(qwant[qi] * 2u))) {
            for (u8 qj = 0; qj < plc.m_quad_n; ++qj) {
                pool_free(&pools[qj]);
            }
            return false;
        }
    }
    u32 rng = seed == 0u ? 1u : seed;
    for (u32 i = 0; i < n; ++i) {
        if (res_ov != nullptr && !tile_open_u16(res_ov, i)) {
            continue;
        }
        for (u8 qi = 0; qi < plc.m_quad_n; ++qi) {
            if (!ResPlcMatch::tile_ok(ctx, i, plc.m_quads[qi])) {
                continue;
            }
            pool_resv(&pools[qi], i, &rng);
        }
    }
    u32 placed = 0;
    for (u8 qi = 0; qi < plc.m_quad_n; ++qi) {
        u32 want = qwant[qi];
        if (qi + 1u == plc.m_quad_n) {
            if (placed + want > soft) {
                want = soft - placed;
            }
        }
        if (res_ov != nullptr) {
            placed += draw_u16(&pools[qi], want, &rng, res_i, res_ov);
        } else {
            placed += draw_u8(&pools[qi], want, &rng, (u8)(qi + 1u), u8_out);
        }
    }
    if (placed < soft) {
        const u32 fill = soft - placed;
        if (res_ov != nullptr) {
            placed += draw_any_u16(pools, plc.m_quad_n, fill, &rng, res_i, res_ov);
        } else {
            placed += draw_any_u8(pools, plc.m_quad_n, fill, &rng, u8_out);
        }
    }
    if (placed < soft && placed < hard) {
        const u32 fill = hard - placed;
        if (res_ov != nullptr) {
            placed += draw_any_u16(pools, plc.m_quad_n, fill, &rng, res_i, res_ov);
        } else {
            placed += draw_any_u8(pools, plc.m_quad_n, fill, &rng, u8_out);
        }
    }
    for (u8 qi = 0; qi < plc.m_quad_n; ++qi) {
        pool_free(&pools[qi]);
    }
    const clock_t t1 = clock();
    if (placed_n != nullptr) {
        *placed_n = placed;
    }
    if (sec_out != nullptr) {
        *sec_out = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    }
    return true;
}

typedef struct FairSlot {
    u16 m_ri;
    u32 m_soft;
    u32 m_hard;
    u32 m_got;
    u32 m_qwant[RES_IO_QUAD_MAX];
    u32 m_qgot[RES_IO_QUAD_MAX];
    const ResPlacement* m_plc;
    ResPlcPool m_pools[RES_IO_QUAD_MAX];
} FairSlot;

static void fair_slot_clr (FairSlot* sl) {
    if (sl == nullptr) {
        return;
    }
    for (u8 qi = 0; qi < RES_IO_QUAD_MAX; ++qi) {
        pool_free(&sl->m_pools[qi]);
        sl->m_qwant[qi] = 0;
        sl->m_qgot[qi] = 0;
    }
    sl->m_plc = nullptr;
    sl->m_got = 0;
}

//================================================================================================================================
//= - R1_Adj_ResPlace -
//================================================================================================================================

R1_Adj_ResPlace::R1_Adj_ResPlace () : m_valid(false) {
}

bool R1_Adj_ResPlace::adjust (
    u16* res_ov,
    u16 w,
    u16 h,
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u32 base_n,
    u32 seed,
    u32* placed_n)
{
    m_valid = false;
    if (res_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (ctx.m_w != w || ctx.m_h != h) {
        return false;
    }
    m_valid = place_core(ctx, s, res_i, base_n, seed, res_ov, nullptr, 0u, placed_n, nullptr);
    return m_valid;
}

bool R1_Adj_ResPlace::is_valid () const {
    return m_valid;
}

bool r1_adj_res_place_u8 (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u32 base_n,
    u32 seed,
    u8* out,
    u32 out_n,
    u32* placed_n,
    double* sec_out)
{
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (out == nullptr || out_n < n) {
        return false;
    }
    return place_core(ctx, s, res_i, base_n, seed, nullptr, out, 0u, placed_n, sec_out);
}

bool r1_adj_res_place_fair (
    u16* res_ov,
    u16 w,
    u16 h,
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    const u16* res_is,
    const u32* soft_want,
    const u32* hard_want,
    u16 res_n,
    u32 seed,
    u32* placed_out,
    u32* placed_tot)
{
    if (placed_tot != nullptr) {
        *placed_tot = 0;
    }
    if (res_ov == nullptr || w == 0 || h == 0 || ctx.m_terrain == nullptr) {
        return false;
    }
    if (ctx.m_w != w || ctx.m_h != h) {
        return false;
    }
    if (res_n == 0) {
        return true;
    }
    if (res_is == nullptr || soft_want == nullptr || hard_want == nullptr || res_n > R1_PLC_FAIR_CAP) {
        return false;
    }
    if (placed_out != nullptr) {
        for (u16 i = 0; i < res_n; ++i) {
            placed_out[i] = 0;
        }
    }
    FairSlot slots[R1_PLC_FAIR_CAP];
    u16 sn = 0;
    for (u16 i = 0; i < res_n; ++i) {
        if (soft_want[i] == 0) {
            continue;
        }
        const ResDistStaticDataStruct* rd = res_rd(s, res_is[i]);
        if (rd == nullptr || rd->has_plc == 0 || rd->plc.m_quad_n == 0) {
            continue;
        }
        const u32 wt_tot = wt_sum(rd->plc);
        if (wt_tot == 0) {
            continue;
        }
        FairSlot& sl = slots[sn];
        sl.m_ri = res_is[i];
        sl.m_soft = soft_want[i];
        sl.m_hard = hard_want[i] < soft_want[i] ? soft_want[i] : hard_want[i];
        sl.m_got = 0;
        sl.m_plc = &rd->plc;
        for (u8 qi = 0; qi < RES_IO_QUAD_MAX; ++qi) {
            sl.m_pools[qi].m_idx = nullptr;
            sl.m_pools[qi].m_n = 0;
            sl.m_pools[qi].m_cap = 0;
            sl.m_pools[qi].m_seen = 0;
            sl.m_qwant[qi] = 0;
            sl.m_qgot[qi] = 0;
        }
        u32 rem_q = 0;
        for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
            sl.m_qwant[qi] = quota_for(sl.m_soft, (u32)rd->plc.m_quads[qi].m_wt, wt_tot, &rem_q);
            if (!pool_init(&sl.m_pools[qi], pool_cap_for(sl.m_hard))) {
                for (u16 j = 0; j <= sn; ++j) {
                    fair_slot_clr(&slots[j]);
                }
                return false;
            }
        }
        sn = static_cast<u16>(sn + 1u);
    }
    if (sn == 0) {
        return true;
    }
    const u32 n = (u32)w * (u32)h;
    u32 rng = seed == 0u ? 1u : seed;
    for (u32 i = 0; i < n; ++i) {
        if (!tile_open_u16(res_ov, i)) {
            continue;
        }
        for (u16 si = 0; si < sn; ++si) {
            FairSlot& sl = slots[si];
            for (u8 qi = 0; qi < sl.m_plc->m_quad_n; ++qi) {
                if (!ResPlcMatch::tile_ok(ctx, i, sl.m_plc->m_quads[qi])) {
                    continue;
                }
                pool_resv(&sl.m_pools[qi], i, &rng);
            }
        }
    }
    bool progress = true;
    while (progress) {
        progress = false;
        for (u16 si = 0; si < sn; ++si) {
            FairSlot& sl = slots[si];
            if (sl.m_got >= sl.m_soft) {
                continue;
            }
            bool drew = false;
            for (u8 qi = 0; qi < sl.m_plc->m_quad_n; ++qi) {
                if (sl.m_qgot[qi] >= sl.m_qwant[qi]) {
                    continue;
                }
                if (draw_u16(&sl.m_pools[qi], 1u, &rng, sl.m_ri, res_ov) == 0) {
                    continue;
                }
                sl.m_qgot[qi] = sl.m_qgot[qi] + 1u;
                sl.m_got = sl.m_got + 1u;
                drew = true;
                progress = true;
                break;
            }
            if (!drew && sl.m_got < sl.m_soft) {
                const u32 d = draw_any_u16(sl.m_pools, sl.m_plc->m_quad_n, 1u, &rng, sl.m_ri, res_ov);
                if (d != 0) {
                    sl.m_got = sl.m_got + d;
                    progress = true;
                }
            }
        }
    }
    progress = true;
    while (progress) {
        progress = false;
        for (u16 si = 0; si < sn; ++si) {
            FairSlot& sl = slots[si];
            if (sl.m_got >= sl.m_soft) {
                continue;
            }
            if (sl.m_got >= sl.m_hard) {
                continue;
            }
            const u32 d = draw_any_u16(sl.m_pools, sl.m_plc->m_quad_n, 1u, &rng, sl.m_ri, res_ov);
            if (d != 0) {
                sl.m_got = sl.m_got + d;
                progress = true;
            }
        }
    }
    u32 tot = 0;
    for (u16 si = 0; si < sn; ++si) {
        for (u16 i = 0; i < res_n; ++i) {
            if (res_is[i] != slots[si].m_ri) {
                continue;
            }
            if (placed_out != nullptr) {
                placed_out[i] = slots[si].m_got;
            }
            break;
        }
        tot += slots[si].m_got;
        fair_slot_clr(&slots[si]);
    }
    if (placed_tot != nullptr) {
        *placed_tot = tot;
    }
    return true;
}

//================================================================================================================================
//= - End -
//================================================================================================================================
