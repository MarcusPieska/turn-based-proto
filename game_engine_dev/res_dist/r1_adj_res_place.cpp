//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "r1_adj_res_place.h"

#include <ctime>

#include "res_dist_static_key.h"
#include "resource_static_key.h"

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

typedef struct ResPlcPool {
    u32* m_idx;
    u32 m_n;
    u32 m_cap;
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
}

static bool pool_add (ResPlcPool* p, u32 idx) {
    if (p == nullptr) {
        return false;
    }
    if (p->m_n >= p->m_cap) {
        const u32 nc = p->m_cap == 0 ? 256u : p->m_cap * 2u;
        u32* ni = new u32[nc];
        if (ni == nullptr) {
            return false;
        }
        for (u32 i = 0; i < p->m_n; ++i) {
            ni[i] = p->m_idx[i];
        }
        delete[] p->m_idx;
        p->m_idx = ni;
        p->m_cap = nc;
    }
    p->m_idx[p->m_n++] = idx;
    return true;
}

static u32 lcg_next (u32* seed) {
    *seed = (*seed * 1103515245u + 12345u);
    return (*seed >> 16u) & 0x7fffu;
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

static u32 draw_u16 (
    ResPlcPool* pool,
    u32 want,
    u32* seed,
    u16 res_i,
    u16* res_ov) 
{
    if (pool == nullptr || res_ov == nullptr || seed == nullptr) {
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
    return placed;
}

static u32 draw_u8 (
    ResPlcPool* pool,
    u32 want,
    u32* seed,
    u8 rule_idx,
    u8* out) 
{
    if (pool == nullptr || out == nullptr || seed == nullptr) {
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
    const u32 wt_tot = wt_sum(rd->plc);
    if (wt_tot == 0) {
        return false;
    }
    const u32 total = base_n * (u32)rd->plc.m_res_wt;
    ResPlcPool* pools = new ResPlcPool[rd->plc.m_quad_n]();
    if (pools == nullptr) {
        return false;
    }
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        pools[qi].m_idx = nullptr;
        pools[qi].m_n = 0;
        pools[qi].m_cap = 0;
    }
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        for (u32 i = 0; i < n; ++i) {
            if (!ResPlcMatch::entry_ok(ctx, i, s, res_i, qi)) {
                continue;
            }
            if (res_ov != nullptr && !tile_open_u16(res_ov, i)) {
                continue;
            }
            if (!pool_add(&pools[qi], i)) {
                for (u8 qj = 0; qj < rd->plc.m_quad_n; ++qj) {
                    pool_free(&pools[qj]);
                }
                delete[] pools;
                return false;
            }
        }
    }
    u32 rem = 0;
    u32 placed = 0;
    u32 rng = seed;
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        const u32 wt = (u32)rd->plc.m_quads[qi].m_wt;
        u32 want = quota_for(total, wt, wt_tot, &rem);
        if (qi + 1u == rd->plc.m_quad_n) {
            if (placed + want > total) {
                want = total - placed;
            }
        }
        if (res_ov != nullptr) {
            placed += draw_u16(&pools[qi], want, &rng, res_i, res_ov);
        } else {
            placed += draw_u8(&pools[qi], want, &rng, (u8)(qi + 1u), u8_out);
        }
    }
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        pool_free(&pools[qi]);
    }
    delete[] pools;
    const clock_t t1 = clock();
    if (placed_n != nullptr) {
        *placed_n = placed;
    }
    if (sec_out != nullptr) {
        *sec_out = (double)(t1 - t0) / (double)CLOCKS_PER_SEC;
    }
    return true;
}

//================================================================================================================================
//=> - R1_Adj_ResPlace -
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
    m_valid = place_core(ctx, s, res_i, base_n, seed, res_ov, nullptr, placed_n, nullptr);
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
    return place_core(ctx, s, res_i, base_n, seed, nullptr, out, placed_n, sec_out);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
