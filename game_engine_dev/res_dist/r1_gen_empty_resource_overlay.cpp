//================================================================================================================================
//= - Includes -
//================================================================================================================================

#include "r1_gen_empty_resource_overlay.h"

#include <cstdio>

#include "game_map_defs.h"

//================================================================================================================================
//= - Helpers -
//================================================================================================================================

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
//= - R1_Gen_EmptyResourceOverlay -
//================================================================================================================================

R1_Gen_EmptyResourceOverlay::R1_Gen_EmptyResourceOverlay ()
    : m_ok(false)
    , m_w(0)
    , m_h(0)
    , m_ov(nullptr)
{
}

R1_Gen_EmptyResourceOverlay::~R1_Gen_EmptyResourceOverlay () {
    clr();
}

void R1_Gen_EmptyResourceOverlay::clr () {
    delete[] m_ov;
    m_ov = nullptr;
    m_ok = false;
    m_w = 0;
    m_h = 0;
}

bool R1_Gen_EmptyResourceOverlay::generate (u16 w, u16 h) {
    clr();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    m_ov = new u16[n];
    if (m_ov == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        m_ov[i] = U16_KEY_NULL;
    }
    m_w = w;
    m_h = h;
    m_ok = true;
    return true;
}

bool R1_Gen_EmptyResourceOverlay::is_valid () const {
    return m_ok;
}

u16 R1_Gen_EmptyResourceOverlay::width () const {
    return m_w;
}

u16 R1_Gen_EmptyResourceOverlay::height () const {
    return m_h;
}

const u16* R1_Gen_EmptyResourceOverlay::overlay () const {
    return m_ov;
}

u16* R1_Gen_EmptyResourceOverlay::overlay_mut () {
    return m_ov;
}

u16* R1_Gen_EmptyResourceOverlay::take_overlay () {
    u16* p = m_ov;
    m_ov = nullptr;
    m_ok = false;
    m_w = 0;
    m_h = 0;
    return p;
}

bool R1_Gen_EmptyResourceOverlay::save_ppm (
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
    (void)res_max;
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
        if (key != U16_KEY_NULL && key <= 255u) {
            r = (u8)key;
            g = (u8)key;
            b = (u8)key;
        }
        const u16 x = (u16)(i % (u32)w);
        const u16 y = (u16)(i / (u32)w);
        set_px(rgb, w, h, x, y, r, g, b);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//= - End of file -
//================================================================================================================================
