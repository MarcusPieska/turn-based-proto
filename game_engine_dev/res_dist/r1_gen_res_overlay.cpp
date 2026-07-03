//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "r1_gen_res_overlay.h"

#include <cerrno>
#include <cstdio>
#include <cstring>

#include <sys/stat.h>

#include "r1_adj_res_place.h"
#include "game_map_defs.h"
#include "generator_constants.h"
#include "resource_static_key.h"

static const char* R1_GEN_RES_OV_OUT_ROOT = "/home/w/Projects/simple-map-gen";

//================================================================================================================================
//=> - Private helpers -
//================================================================================================================================

static bool ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    if (mkdir(path, 0755) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static bool is_water (u8 t) {
    return t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0];
}

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (u32)y * (u32)w + (u32)x;
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
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

//================================================================================================================================
//=> - R1_Gen_ResOverlay -
//================================================================================================================================

R1_Gen_ResOverlay::R1_Gen_ResOverlay () :
    m_valid(false),
    m_w(0),
    m_h(0),
    m_ov(nullptr) {
}

R1_Gen_ResOverlay::~R1_Gen_ResOverlay () {
    delete[] m_ov;
    m_ov = nullptr;
}

bool R1_Gen_ResOverlay::generate (
    const ResPlcMapCtx& ctx,
    const RuntimeStatics& s,
    u32 base_n,
    u32 seed) 
{
    m_valid = false;
    delete[] m_ov;
    m_ov = nullptr;
    m_w = ctx.m_w;
    m_h = ctx.m_h;
    if (m_w == 0 || m_h == 0 || ctx.m_terrain == nullptr || base_n == 0) {
        return false;
    }
    const u32 n = (u32)m_w * (u32)m_h;
    m_ov = new u16[n];
    if (m_ov == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        m_ov[i] = U16_KEY_NULL;
    }
    const u16 res_n = s.resource().get_item_count();
    R1_Adj_ResPlace adj;
    for (u16 ri = 0; ri < res_n; ++ri) {
        const ResourceStaticDataStruct& r = s.resource().get_item(
            ResourceStaticDataKey::from_raw(ri));
        if (r.res_dist_idx >= s.res_dist().get_item_count()) {
            continue;
        }
        const ResDistStaticDataStruct& rd = s.res_dist().get_item(
            ResDistStaticDataKey::from_raw(r.res_dist_idx));
        if (rd.has_plc == 0) {
            continue;
        }
        if (!adj.adjust(m_ov, m_w, m_h, ctx, s, ri, base_n, seed + (u32)ri, nullptr)) {
            return false;
        }
    }
    m_valid = true;
    return true;
}

bool R1_Gen_ResOverlay::is_valid () const {
    return m_valid;
}

u16 R1_Gen_ResOverlay::width () const {
    return m_w;
}

u16 R1_Gen_ResOverlay::height () const {
    return m_h;
}

const u16* R1_Gen_ResOverlay::overlay () const {
    return m_ov;
}

u16* R1_Gen_ResOverlay::take_overlay () {
    u16* p = m_ov;
    m_ov = nullptr;
    m_valid = false;
    m_w = 0;
    m_h = 0;
    return p;
}

bool R1_Gen_ResOverlay::save_ppm (
    cstr path,
    const ResPlcMapCtx& ctx,
    const u16* res_ov,
    u16 w,
    u16 h,
    u16 res_max) 
{
    if (path == nullptr || res_ov == nullptr || ctx.m_terrain == nullptr) {
        return false;
    }
    if (w == 0 || h == 0 || ctx.m_w != w || ctx.m_h != h) {
        return false;
    }
    const u32 n = (u32)w * (u32)h;
    u8* rgb = new u8[n * 3u];
    if (rgb == nullptr) {
        return false;
    }
    const u16 denom = res_max > 0 ? res_max : 1u;
    for (u32 i = 0; i < n; ++i) {
        u8 r = 80;
        u8 g = 160;
        u8 b = 80;
        if (is_water(ctx.m_terrain[i])) {
            r = 64;
            g = 128;
            b = 192;
        }
        const u16 key = res_ov[i];
        if (key != U16_KEY_NULL) {
            const u32 gray = ((u32)key * 255u) / (u32)denom;
            r = (u8)gray;
            g = (u8)gray;
            b = (u8)gray;
        }
        const u16 x = (u16)(i % (u32)w);
        const u16 y = (u16)(i / (u32)w);
        set_px(rgb, w, h, x, y, r, g, b);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool R1_Gen_ResOverlay::make_out_path (u32 seed, cstr fname, char* out, u32 cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    if (!ensure_dir(R1_GEN_RES_OV_OUT_ROOT)) {
        return false;
    }
    char seed_dir[384];
    const int seed_n = std::snprintf(seed_dir, sizeof(seed_dir), "%s/p1-seed-%03u",
        R1_GEN_RES_OV_OUT_ROOT, seed);
    if (seed_n < 0 || (u32)seed_n >= sizeof(seed_dir)) {
        return false;
    }
    if (!ensure_dir(seed_dir)) {
        return false;
    }
    const int out_n = std::snprintf(out, cap, "%s/%s", seed_dir, fname);
    if (out_n < 0 || (u32)out_n >= cap) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
