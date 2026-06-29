//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <sys/stat.h>

#include "resource_placement.h"
#include "resource_types.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"

static const char* RES_PLC_OUT_ROOT = "/home/w/Projects/simple-map-gen";
static const char* RES_PLC_OUT_SUBDIR = "resource_placement";

//================================================================================================================================
//=> - ResPlcMatch -
//================================================================================================================================

static bool terr_ok (u8 tile, u8 req) {
    if (req == RES_TERR_ALL) {
        return true;
    }
    return tile == req;
}

static bool clim_ok (u8 tile, u8 req) {
    if (req == RES_CLIM_ALL) {
        return true;
    }
    return tile == req;
}

static bool ov_ok (u8 tile, u8 req) {
    if (req == RES_OV_ALL) {
        return true;
    }
    if (req == RES_OV_NONE) {
        return tile == 0;
    }
    return tile == req;
}

bool ResPlcMatch::tile_ok (
    const ResPlcMapCtx& ctx,
    u32 idx,
    const ResQuad& q) 
{
    if (ctx.m_terrain == nullptr || idx >= (u32)ctx.m_w * (u32)ctx.m_h) {
        return false;
    }
    const u8 ov = ctx.m_overlay != nullptr ? ctx.m_overlay[idx] : 0;
    const u8 cl = ctx.m_climate != nullptr ? ctx.m_climate[idx] : CLIMATE_NONE;
    return terr_ok(ctx.m_terrain[idx], q.m_terr)
        && clim_ok(cl, q.m_clim)
        && ov_ok(ov, q.m_ov);
}

bool ResPlcMatch::entry_ok (
    const ResPlcMapCtx& ctx,
    u32 idx,
    const ResEntry& entry,
    u8 quad_idx) 
{
    if (entry.m_has_plc == 0 || quad_idx >= entry.m_plc.m_quad_n) {
        return false;
    }
    return tile_ok(ctx, idx, entry.m_plc.m_quads[quad_idx]);
}

u32 ResPlcMatch::mark_all_rules (
    const ResPlcMapCtx& ctx,
    const ResEntry& entry,
    u8* out,
    u32 out_n) 
{
    if (out == nullptr || entry.m_has_plc == 0) {
        return 0;
    }
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (out_n < n) {
        return 0;
    }
    u32 hit = 0;
    for (u32 i = 0; i < n; ++i) {
        out[i] = 0;
    }
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
        for (u32 i = 0; i < n; ++i) {
            if (ResPlcMatch::entry_ok(ctx, i, entry, qi)) {
                out[i] = (u8)(qi + 1u);
                ++hit;
            }
        }
    }
    return hit;
}

//================================================================================================================================
//=> - ResPlcSelect -
//================================================================================================================================

typedef struct ResPlcPool {
    u32* m_idx;
    u32 m_n;
    u32 m_cap;
} ResPlcPool;

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

static u32 wt_sum (const ResEntry& entry) {
    u32 s = 0;
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
        s += (u32)entry.m_plc.m_quads[qi].m_wt;
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

static u32 draw_from_pool (
    ResPlcPool* pool,
    u32 want,
    u32* seed,
    u8 rule_idx,
    u8* out,
    u32 n) 
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
        if (out[idx] != 0) {
            continue;
        }
        out[idx] = rule_idx;
        ++placed;
    }
    return placed;
}

bool ResPlcSelect::run (
    const ResPlcMapCtx& ctx,
    const ResEntry& entry,
    u32 base_n,
    u32 seed,
    u8* out,
    u32 out_n,
    u32* placed_n,
    double* sec_out) 
{
    if (placed_n != nullptr) {
        *placed_n = 0;
    }
    if (sec_out != nullptr) {
        *sec_out = 0.0;
    }
    if (out == nullptr || entry.m_has_plc == 0) {
        return false;
    }
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (out_n < n || base_n == 0) {
        return false;
    }
    const clock_t t0 = clock();
    for (u32 i = 0; i < n; ++i) {
        out[i] = 0;
    }
    const u32 wt_tot = wt_sum(entry);
    if (wt_tot == 0) {
        return false;
    }
    const u32 total = base_n * (u32)entry.m_plc.m_res_wt;
    ResPlcPool* pools = new ResPlcPool[entry.m_plc.m_quad_n]();
    if (pools == nullptr) {
        return false;
    }
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
        pools[qi].m_idx = nullptr;
        pools[qi].m_n = 0;
        pools[qi].m_cap = 0;
    }
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
        for (u32 i = 0; i < n; ++i) {
            if (ResPlcMatch::entry_ok(ctx, i, entry, qi)) {
                if (!pool_add(&pools[qi], i)) {
                    for (u8 qj = 0; qj < entry.m_plc.m_quad_n; ++qj) {
                        pool_free(&pools[qj]);
                    }
                    delete[] pools;
                    return false;
                }
            }
        }
    }
    u32 rem = 0;
    u32 placed = 0;
    u32 rng = seed;
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
        const u32 wt = (u32)entry.m_plc.m_quads[qi].m_wt;
        u32 want = quota_for(total, wt, wt_tot, &rem);
        if (qi + 1u == entry.m_plc.m_quad_n) {
            if (placed + want > total) {
                want = total - placed;
            }
        }
        placed += draw_from_pool(&pools[qi], want, &rng, (u8)(qi + 1u), out, n);
    }
    for (u8 qi = 0; qi < entry.m_plc.m_quad_n; ++qi) {
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
//=> - ResPlcOverlay -
//================================================================================================================================

u8* ResPlcOverlay::build_stub (
    u16 w,
    u16 h,
    const u8* terrain,
    const u8* climate,
    const u8* river) 
{
    const u32 n = (u32)w * (u32)h;
    u8* ov = new u8[n]();
    if (ov == nullptr || terrain == nullptr || climate == nullptr) {
        delete[] ov;
        return nullptr;
    }
    for (u32 i = 0; i < n; ++i) {
        if (river != nullptr && river[i] != 0) {
            ov[i] = RES_OV_RIVERS;
            continue;
        }
        const u8 t = terrain[i];
        const u8 c = climate[i];
        if (c == CLIMATE_GRASSLAND
            && (t == TERR_PLAINS[0] || t == TERR_HILLS[0])
            && (i % 7u) != 0u) {
            ov[i] = RES_OV_FORESTS;
            continue;
        }
        if (c == CLIMATE_GRASSLAND && t == TERR_PLAINS[0] && (i % 11u) == 0u) {
            ov[i] = RES_OV_JUNGLES;
            continue;
        }
        if (c == CLIMATE_GRASSLAND && t == TERR_PLAINS[0] && (i % 13u) == 0u) {
            ov[i] = RES_OV_SWAMPS;
        }
    }
    return ov;
}

//================================================================================================================================
//=> - ResPlcViz -
//================================================================================================================================

void ResPlcViz::rule_rgb (u8 rule_idx, u8* r, u8* g, u8* b) {
    switch (rule_idx) {
    case 1:
        *r = 255;
        *g = 40;
        *b = 40;
        return;
    case 2:
        *r = 180;
        *g = 40;
        *b = 255;
        return;
    case 3:
        *r = 255;
        *g = 105;
        *b = 180;
        return;
    case 4:
        *r = 255;
        *g = 255;
        *b = 0;
        return;
    default:
        *r = 255;
        *g = 255;
        *b = 255;
        return;
    }
}

static bool ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static void climate_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (cls == CLIMATE_GRASSLAND) {
        *r = 90;
        *g = 170;
        *b = 50;
        return;
    }
    if (cls == CLIMATE_PLAINS) {
        *r = 210;
        *g = 200;
        *b = 80;
        return;
    }
    if (cls == CLIMATE_DESERT) {
        *r = 210;
        *g = 160;
        *b = 70;
        return;
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static void soften_bg_px (u8* r, u8* g, u8* b) {
    const u32 gray = (77u * (u32)(*r) + 150u * (u32)(*g) + 29u * (u32)(*b)) >> 8;
    const u32 desat = 220u;
    *r = (u8)(((u32)(*r) * (256u - desat) + gray * desat) >> 8);
    *g = (u8)(((u32)(*g) * (256u - desat) + gray * desat) >> 8);
    *b = (u8)(((u32)(*b) * (256u - desat) + gray * desat) >> 8);
    const u32 bleach = 120u;
    *r = (u8)(((u32)(*r) * (256u - bleach) + 255u * bleach) >> 8);
    *g = (u8)(((u32)(*g) * (256u - bleach) + 255u * bleach) >> 8);
    *b = (u8)(((u32)(*b) * (256u - bleach) + 255u * bleach) >> 8);
}

static void set_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= (i32)w || y >= (i32)h) {
        return;
    }
    const u32 i = (u32)y * (u32)w + (u32)x;
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void draw_mark (
    u8* rgb,
    u16 w,
    u16 h,
    u16 x0,
    u16 y,
    u16 x,
    u8 rule_idx,
    u8 dot_rad) 
{
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    ResPlcViz::rule_rgb(rule_idx, &r, &g, &b);
    if (dot_rad == 0) {
        set_px(rgb, w, h, (i32)x0 + (i32)x, (i32)y, r, g, b);
        return;
    }
    const i32 rad = (i32)dot_rad;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            set_px(rgb, w, h, (i32)x0 + (i32)x + dx, (i32)y + dy, r, g, b);
        }
    }
}

static void fill_bg (
    const ResPlcMapCtx& ctx,
    u8* rgb,
    u16 out_w,
    u16 x0,
    bool use_climate) 
{
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(ctx.m_terrain[i], &r, &g, &b);
        if (use_climate && ctx.m_climate != nullptr) {
            const u8 cl = ctx.m_climate[i];
            if (cl != CLIMATE_NONE) {
                climate_rgb(cl, &r, &g, &b);
            }
            if (ctx.m_terrain[i] == TERR_MOUNTAINS[0]) {
                MapTerrainValidate::rgb_from_class(TERR_MOUNTAINS[0], &r, &g, &b);
            }
            if (ctx.m_river != nullptr && ctx.m_river[i] != 0) {
                r = 0;
                g = 0;
                b = 255;
            }
        }
        soften_bg_px(&r, &g, &b);
        const u16 x = (u16)(i % (u32)ctx.m_w);
        const u16 y = (u16)(i / (u32)ctx.m_w);
        set_px(rgb, out_w, ctx.m_h, (i32)x0 + (i32)x, (i32)y, r, g, b);
    }
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

bool ResPlcViz::save_pair_img (
    cstr path,
    const ResPlcMapCtx& ctx,
    const u8* marks,
    u32 mark_n,
    const ResPlcVizPrm& prm) 
{
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (path == nullptr || marks == nullptr || mark_n < n) {
        return false;
    }
    const u16 out_w = (u16)(ctx.m_w * 2u);
    u8* rgb = new u8[static_cast<size_t>(out_w) * static_cast<size_t>(ctx.m_h) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    fill_bg(ctx, rgb, out_w, 0, false);
    fill_bg(ctx, rgb, out_w, ctx.m_w, true);
    for (u32 i = 0; i < n; ++i) {
        if (marks[i] == 0) {
            continue;
        }
        const u16 x = (u16)(i % (u32)ctx.m_w);
        const u16 y = (u16)(i / (u32)ctx.m_w);
        draw_mark(rgb, out_w, ctx.m_h, 0, y, x, marks[i], prm.m_dot_rad);
        draw_mark(rgb, out_w, ctx.m_h, ctx.m_w, y, x, marks[i], prm.m_dot_rad);
    }
    const bool ok = save_rgb_ppm(path, rgb, out_w, ctx.m_h);
    delete[] rgb;
    return ok;
}

bool ResPlcViz::make_out_path (u32 seed, cstr fname, char* out, u32 cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!ensure_dir(RES_PLC_OUT_ROOT)) {
        return false;
    }
    char seed_dir[384];
    const int seed_n = std::snprintf(seed_dir, sizeof(seed_dir), "%s/p1-seed-%03u",
        RES_PLC_OUT_ROOT, seed);
    if (seed_n < 0 || (u32)seed_n >= sizeof(seed_dir)) {
        return false;
    }
    if (!ensure_dir(seed_dir)) {
        return false;
    }
    char dir[384];
    const int dir_n = std::snprintf(dir, sizeof(dir), "%s/%s", seed_dir, RES_PLC_OUT_SUBDIR);
    if (dir_n < 0 || (u32)dir_n >= sizeof(dir)) {
        return false;
    }
    if (!ensure_dir(dir)) {
        return false;
    }
    const int out_n = std::snprintf(out, cap, "%s/%s", dir, fname);
    if (out_n < 0 || (u32)out_n >= cap) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
