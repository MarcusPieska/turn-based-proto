//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cmath>

#include "game_primitives.h"
#include "p1_gen_wind_pattern_adv.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const u16 g_arrow_blk = 20u;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (rgb == nullptr || x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static const i8 k_gt_off[] = {
    2, -3,
    3, -2,
    4, 0,
    3, 2,
    2, 3,
};

static void stamp_gt_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    put_px(rgb, w, h, x, y, r, g, b);
    put_px(rgb, w, h, x + 1, y, r, g, b);
    put_px(rgb, w, h, x, y + 1, r, g, b);
    put_px(rgb, w, h, x + 1, y + 1, r, g, b);
}

static void draw_gt_at (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 dir, u8 str) {
    if (str < 8) {
        return;
    }
    const f32 ang = (static_cast<f32>(dir) / 255.f) * 6.28318530f;
    const f32 ca = std::cos(ang);
    const f32 sa = std::sin(ang);
    const f32 sc = 1.05f + 1.05f * (static_cast<f32>(str) / 255.f);
    const size_t pt_n = sizeof(k_gt_off) / sizeof(k_gt_off[0]) / 2u;
    i32 bx[12];
    i32 by[12];
    i32 fx[12];
    i32 fy[12];
    size_t pn = 0;
    for (size_t p = 0; p < pt_n; ++p) {
        const f32 ox = static_cast<f32>(k_gt_off[p * 2u + 0u]) * sc;
        const f32 oy = static_cast<f32>(k_gt_off[p * 2u + 1u]) * sc;
        const f32 rx = ox * ca - oy * sa;
        const f32 ry = ox * sa + oy * ca;
        const i32 x = static_cast<i32>(static_cast<f32>(cx) + rx + 0.5f);
        const i32 y = static_cast<i32>(static_cast<f32>(cy) + ry + 0.5f);
        if (pn < 12) {
            bx[pn] = x;
            by[pn] = y;
            fx[pn] = x;
            fy[pn] = y;
            pn++;
        }
    }
    for (size_t p = 0; p < pn; ++p) {
        stamp_gt_px(rgb, w, h, bx[p] - 1, by[p], 0, 0, 0);
        stamp_gt_px(rgb, w, h, bx[p] + 1, by[p], 0, 0, 0);
        stamp_gt_px(rgb, w, h, bx[p], by[p] - 1, 0, 0, 0);
        stamp_gt_px(rgb, w, h, bx[p], by[p] + 1, 0, 0, 0);
    }
    for (size_t p = 0; p < pn; ++p) {
        stamp_gt_px(rgb, w, h, fx[p], fy[p], 255, 178, 178);
    }
}

static void draw_arrow_grid (u8* rgb, u16 w, u16 h, const u8* dir, const u8* str) {
    if (rgb == nullptr || dir == nullptr || str == nullptr || g_arrow_blk == 0) {
        return;
    }
    const u16 bw = static_cast<u16>((static_cast<u32>(w) + static_cast<u32>(g_arrow_blk) - 1u)
        / static_cast<u32>(g_arrow_blk));
    const u16 bh = static_cast<u16>((static_cast<u32>(h) + static_cast<u32>(g_arrow_blk) - 1u)
        / static_cast<u32>(g_arrow_blk));
    for (u16 by = 0; by < bh; ++by) {
        for (u16 bx = 0; bx < bw; ++bx) {
            u32 px_u = static_cast<u32>(bx) * static_cast<u32>(g_arrow_blk)
                + static_cast<u32>(g_arrow_blk) / 2u;
            u32 py_u = static_cast<u32>(by) * static_cast<u32>(g_arrow_blk)
                + static_cast<u32>(g_arrow_blk) / 2u;
            if (px_u >= static_cast<u32>(w)) {
                px_u = static_cast<u32>(w) - 1u;
            }
            if (py_u >= static_cast<u32>(h)) {
                py_u = static_cast<u32>(h) - 1u;
            }
            const u16 cx = static_cast<u16>(px_u);
            const u16 cy = static_cast<u16>(py_u);
            const u32 ti = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
            draw_gt_at(rgb, w, h, cx, cy, dir[ti], str[ti]);
        }
    }
}

static void dir_to_rgb (u8 dir, u8 str, u8* r, u8* g, u8* b) {
    const f32 ang = (static_cast<f32>(dir) / 255.f) * 6.28318530f;
    const f32 hx = ang / 6.28318530f;
    i32 sector = static_cast<i32>(hx * 6.f);
    if (sector < 0) {
        sector = 0;
    }
    if (sector > 5) {
        sector = 5;
    }
    const f32 f = hx * 6.f - static_cast<f32>(sector);
    f32 p = 0.f;
    f32 q = 1.f - f;
    f32 t = f;
    f32 rf = 0.f;
    f32 gf = 0.f;
    f32 bf = 0.f;
    switch (sector) {
        case 0: rf = 1.f; gf = t; bf = p; break;
        case 1: rf = q; gf = 1.f; bf = p; break;
        case 2: rf = p; gf = 1.f; bf = t; break;
        case 3: rf = p; gf = q; bf = 1.f; break;
        case 4: rf = t; gf = p; bf = 1.f; break;
        default: rf = 1.f; gf = p; bf = q; break;
    }
    const f32 dim = 0.25f + 0.75f * (static_cast<f32>(str) / 255.f);
    if (r != nullptr) {
        *r = static_cast<u8>(rf * dim * 255.f);
    }
    if (g != nullptr) {
        *g = static_cast<u8>(gf * dim * 255.f);
    }
    if (b != nullptr) {
        *b = static_cast<u8>(bf * dim * 255.f);
    }
}

static bool save_dir_viz (cstr path, const u8* dir, const u8* str, u16 w, u16 h) {
    if (path == nullptr || dir == nullptr || str == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        dir_to_rgb(dir[i], str[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_str_viz (cstr path, const u8* dir, const u8* str, u16 w, u16 h) {
    if (path == nullptr || dir == nullptr || str == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 v = str[i];
        rgb[i * 3u + 0] = v;
        rgb[i * 3u + 1] = v;
        rgb[i * 3u + 2] = v;
    }
    draw_arrow_grid(rgb, w, h, dir, str);
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool run_chunk (
    P1_TesterHarness& h,
    const u8* terrain,
    u16 w,
    u16 hgt,
    u16 chunk_sz) 
{
    char dir_path[320];
    char str_path[320];
    char dir_suffix[160];
    char str_suffix[160];
    std::snprintf(dir_suffix, sizeof(dir_suffix), "wind_pattern_adv_dir_c%02u", static_cast<unsigned>(chunk_sz));
    std::snprintf(str_suffix, sizeof(str_suffix), "wind_pattern_adv_str_c%02u", static_cast<unsigned>(chunk_sz));
    const u16 def_chunk = p1_gen_wind_pattern_adv_prm_def().m_chunk_sz;
    const bool is_def = chunk_sz == def_chunk;
    if (is_def) {
        if (!h.path_pri(dir_path, sizeof(dir_path)) || !h.path_sec(str_path, sizeof(str_path))) {
            std::printf("failed to ensure output dir for chunk %u\n", static_cast<unsigned>(chunk_sz));
            return false;
        }
    } else if (!h.path_extra(dir_suffix, dir_path, sizeof(dir_path))
        || !h.path_extra(str_suffix, str_path, sizeof(str_path))) {
        std::printf("failed to ensure output dir for chunk %u\n", static_cast<unsigned>(chunk_sz));
        return false;
    }
    P1_Gen_WindPatternAdvPrm sp = p1_gen_wind_pattern_adv_prm_def();
    sp.m_chunk_sz = chunk_sz;
    P1_Gen_WindPatternAdv gen(h.prm(), sp);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, hgt);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_WindPatternAdv failed chunk=%u\n", static_cast<unsigned>(chunk_sz));
        return false;
    }
    const P1_Gen_WindPatternAdvRslt& r = gen.result();
    const u8* dir = r.m_dir.data();
    const u8* str = r.m_str.data();
    if (dir == nullptr || str == nullptr) {
        std::printf("P1_Gen_WindPatternAdv missing overlays chunk=%u\n", static_cast<unsigned>(chunk_sz));
        return false;
    }
    std::printf("P1_Gen_WindPatternAdv chunk=%2u generate time: %.6f s (%u x %u)\n",
        static_cast<unsigned>(chunk_sz),
        sec,
        static_cast<u32>(w),
        static_cast<u32>(hgt));
    if (!save_dir_viz(dir_path, dir, str, w, hgt)) {
        std::printf("failed to save dir viz: %s\n", dir_path);
        return false;
    }
    if (!save_str_viz(str_path, dir, str, w, hgt)) {
        std::printf("failed to save str viz: %s\n", str_path);
        return false;
    }
    std::printf("saved: %s\n", dir_path);
    std::printf("saved: %s\n", str_path);
    return true;
}

i32 test_p1_gen_wind_pattern_adv_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        std::printf("P1 steps 1-21 input failed for step 22\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u8* terrain = chain.m_terrain;
    if (terrain == nullptr || w == 0 || ht == 0) {
        std::printf("invalid chain terrain\n");
        return -1;
    }
    std::printf("P1 steps 1-21 input time: %.6f s\n", h.input_sec());
    static const u16 k_chunks[] = {10u};
    for (size_t i = 0; i < sizeof(k_chunks) / sizeof(k_chunks[0]); ++i) {
        if (!run_chunk(h, terrain, w, ht, k_chunks[i])) {
            return -1;
        }
    }
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_p1_gen_wind_pattern_adv_basic(h);
    if (rc != 0) {
        return rc;
    }
    if (!h.finish()) {
        return -1;
    }
    return 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
