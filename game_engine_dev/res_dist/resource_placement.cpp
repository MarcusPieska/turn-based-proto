//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <ctime>

#include <sys/stat.h>

#include "resource_placement.h"
#include "r1_adj_res_place.h"
#include "res_placement_defs.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"
#include "resource_static_key.h"
#include "res_dist_static_key.h"

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

bool ResPlcMatch::entry_ok (
    const ResPlcMapCtx& ctx,
    u32 idx,
    const RuntimeStatics& s,
    u16 res_i,
    u8 quad_idx) 
{
    const ResDistStaticDataStruct* rd = res_rd(s, res_i);
    if (rd == nullptr || rd->has_plc == 0 || quad_idx >= rd->plc.m_quad_n) {
        return false;
    }
    return tile_ok(ctx, idx, rd->plc.m_quads[quad_idx]);
}

u32 ResPlcMatch::mark_all_rules (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u16 res_i,
    u8* out,
    u32 out_n) 
{
    const ResDistStaticDataStruct* rd = res_rd(s, res_i);
    if (out == nullptr || rd == nullptr || rd->has_plc == 0) {
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
    for (u8 qi = 0; qi < rd->plc.m_quad_n; ++qi) {
        for (u32 i = 0; i < n; ++i) {
            if (ResPlcMatch::entry_ok(ctx, i, s, res_i, qi)) {
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

bool ResPlcSelect::run (
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
    return r1_adj_res_place_u8(ctx, s, res_i, base_n, seed, out, out_n, placed_n, sec_out);
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
    u16 y0,
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
        set_px(rgb, w, h, (i32)x0 + (i32)x, (i32)y0 + (i32)y, r, g, b);
        return;
    }
    const i32 rad = (i32)dot_rad;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            set_px(rgb, w, h, (i32)x0 + (i32)x + dx, (i32)y0 + (i32)y + dy, r, g, b);
        }
    }
}

static void draw_marks_panel (
    u8* rgb,
    u16 out_w,
    u16 out_h,
    u16 map_w,
    u16 map_h,
    u16 x0,
    u16 y0,
    const u8* marks,
    u32 mark_n,
    u8 dot_rad) 
{
    const u32 n = (u32)map_w * (u32)map_h;
    if (marks == nullptr || mark_n < n) {
        return;
    }
    for (u32 i = 0; i < n; ++i) {
        if (marks[i] == 0) {
            continue;
        }
        const u16 x = (u16)(i % (u32)map_w);
        const u16 y = (u16)(i / (u32)map_w);
        draw_mark(rgb, out_w, out_h, x0, y0, y, x, marks[i], dot_rad);
    }
}

static void draw_marks_pair (
    const ResPlcMapCtx& ctx,
    u8* rgb,
    u16 out_w,
    u16 out_h,
    u16 y0,
    const u8* marks,
    u32 mark_n,
    u8 dot_rad) 
{
    draw_marks_panel(rgb, out_w, out_h, ctx.m_w, ctx.m_h, 0, y0, marks, mark_n, dot_rad);
    draw_marks_panel(rgb, out_w, out_h, ctx.m_w, ctx.m_h, ctx.m_w, y0, marks, mark_n, dot_rad);
}

static void fill_bg (
    const ResPlcMapCtx& ctx,
    u8* rgb,
    u16 out_w,
    u16 out_h,
    u16 x0,
    u16 y0,
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
        set_px(rgb, out_w, out_h, (i32)x0 + (i32)x, (i32)y0 + (i32)y, r, g, b);
    }
}

static void fill_pair_row (
    const ResPlcMapCtx& ctx,
    u8* rgb,
    u16 out_w,
    u16 out_h,
    u16 y0) 
{
    fill_bg(ctx, rgb, out_w, out_h, 0, y0, false);
    fill_bg(ctx, rgb, out_w, out_h, ctx.m_w, y0, true);
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
    const u16 out_h = ctx.m_h;
    u8* rgb = new u8[static_cast<size_t>(out_w) * static_cast<size_t>(out_h) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    fill_pair_row(ctx, rgb, out_w, out_h, 0);
    for (u32 i = 0; i < n; ++i) {
        if (marks[i] == 0) {
            continue;
        }
        const u16 x = (u16)(i % (u32)ctx.m_w);
        const u16 y = (u16)(i / (u32)ctx.m_w);
        draw_mark(rgb, out_w, out_h, 0, 0, y, x, marks[i], prm.m_dot_rad);
        draw_mark(rgb, out_w, out_h, ctx.m_w, 0, y, x, marks[i], prm.m_dot_rad);
    }
    const bool ok = save_rgb_ppm(path, rgb, out_w, out_h);
    delete[] rgb;
    return ok;
}

bool ResPlcViz::save_pair_ov_img (
    cstr path,
    const ResPlcMapCtx& ctx,
    const u8* poss_marks,
    const u8* act_marks,
    u32 mark_n,
    const ResPlcVizPrm& prm) 
{
    const u32 n = (u32)ctx.m_w * (u32)ctx.m_h;
    if (path == nullptr || poss_marks == nullptr || act_marks == nullptr || mark_n < n) {
        return false;
    }
    const u16 out_w = (u16)(ctx.m_w * 2u);
    const u16 out_h = (u16)(ctx.m_h * 2u);
    u8* rgb = new u8[static_cast<size_t>(out_w) * static_cast<size_t>(out_h) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    fill_pair_row(ctx, rgb, out_w, out_h, 0);
    fill_pair_row(ctx, rgb, out_w, out_h, ctx.m_h);
    draw_marks_pair(ctx, rgb, out_w, out_h, 0, poss_marks, mark_n, prm.m_dot_rad);
    draw_marks_pair(ctx, rgb, out_w, out_h, ctx.m_h, act_marks, mark_n, res_plc_viz_prm_def().m_dot_rad);
    const bool ok = save_rgb_ppm(path, rgb, out_w, out_h);
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
