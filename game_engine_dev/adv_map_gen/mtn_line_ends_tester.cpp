//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "mtn_line_ends.h"
#include "game_map_defs.h"
#include "game_primitives.h"
#include "p1_make_map.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

#ifndef MTN_LINE_ENDS_VIZ_CIRCLES
#define MTN_LINE_ENDS_VIZ_CIRCLES 0
#endif

static void blend_px (u8* rgb, u32 p, u8 sr, u8 sg, u8 sb, u8 a) {
    const u8 ia = static_cast<u8>(255u - a);
    rgb[p + 0] = static_cast<u8>((static_cast<u16>(rgb[p + 0]) * ia + static_cast<u16>(sr) * a) / 255u);
    rgb[p + 1] = static_cast<u8>((static_cast<u16>(rgb[p + 1]) * ia + static_cast<u16>(sg) * a) / 255u);
    rgb[p + 2] = static_cast<u8>((static_cast<u16>(rgb[p + 2]) * ia + static_cast<u16>(sb) * a) / 255u);
}

static void draw_mark (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 sr, u8 sg, u8 sb) {
    if (MTN_LINE_ENDS_VIZ_CIRCLES) {
        const i32 rad = 10;
        const i32 r2 = rad * rad;
        const u8 a = 140u;
        const i32 wi = static_cast<i32>(w);
        const i32 hi = static_cast<i32>(h);
        for (i32 dy = -rad; dy <= rad; ++dy) {
            for (i32 dx = -rad; dx <= rad; ++dx) {
                if (dx * dx + dy * dy > r2) {
                    continue;
                }
                const i32 x = static_cast<i32>(cx) + dx;
                const i32 y = static_cast<i32>(cy) + dy;
                if (x < 0 || y < 0 || x >= wi || y >= hi) {
                    continue;
                }
                const u32 p = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
                blend_px(rgb, p, sr, sg, sb, a);
            }
        }
        return;
    }
    const u32 p = (static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx)) * 3u;
    rgb[p + 0] = sr;
    rgb[p + 1] = sg;
    rgb[p + 2] = sb;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
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

static bool save_ends_viz (
    cstr path,
    const u8* terrain,
    const u16* line_ov,
    u16 w,
    u16 h,
    const MtnLineEndPair* pairs,
    u32 pair_n) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 144u;
        u8 g = 200u;
        u8 b = 120u;
        if (overlay_is_water_terr(terrain[i])) {
            r = 40u;
            g = 90u;
            b = 200u;
        }
        if (line_ov[i] != 0u) {
            r = 0u;
            g = 0u;
            b = 0u;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 i = 0; i < pair_n; ++i) {
        const u32 sp = (static_cast<u32>(pairs[i].m_sy) * static_cast<u32>(w) + static_cast<u32>(pairs[i].m_sx)) * 3u;
        rgb[sp + 0] = 255u;
        rgb[sp + 1] = 255u;
        rgb[sp + 2] = 255u;
    }
    for (u32 i = 0; i < pair_n; ++i) {
        draw_mark(rgb, w, h, pairs[i].m_ax, pairs[i].m_ay, 0u, 220u, 255u);
        draw_mark(rgb, w, h, pairs[i].m_bx, pairs[i].m_by, 255u, 0u, 0u);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_mtn_line_ends_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_make_final_export_path(prm.m_seed, "mtn_line_ends", out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const clock_t t0i = clock();
    P1_MakeMap mk(prm);
    const bool mk_ok = mk.generate(k_p1_step_foothills);
    const clock_t t1i = clock();
    if (!mk_ok || !mk.is_valid()) {
        std::printf("P1_MakeMap failed for mtn line ends input\n");
        return -1;
    }
    const P1_MakeMapRslt& mr = mk.result();
    if (mr.m_flags == nullptr || mr.m_terrain == nullptr) {
        std::printf("P1_MakeMap missing flags or terrain\n");
        return -1;
    }
    const u16 w = mr.m_w;
    const u16 h = mr.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u16* line_ov = new u16[npx];
    MtnLineEndPair* pairs = new MtnLineEndPair[npx];
    if (line_ov == nullptr || pairs == nullptr) {
        delete[] line_ov;
        delete[] pairs;
        return -1;
    }
    u32 line_tile_n = 0u;
    for (u32 i = 0; i < npx; ++i) {
        const u16 v = (mr.m_flags[i].defensible_mtn != 0u) ? 1u : 0u;
        line_ov[i] = v;
        line_tile_n += v;
    }
    MtnLineEnds ends(prm.m_seed);
    if (!ends.ok()) {
        std::printf("MtnLineEnds whiteboard checkout failed\n");
        delete[] line_ov;
        delete[] pairs;
        return -1;
    }
    const clock_t t0 = clock();
    if (!ends.begin(line_ov, w, h)) {
        std::printf("MtnLineEnds begin failed\n");
        delete[] line_ov;
        delete[] pairs;
        return -1;
    }
    u32 pair_n = 0u;
    MtnLineEndPair pair;
    while (ends.next(&pair)) {
        if (mtn_line_end_pair_is_nil(pair)) {
            break;
        }
        pairs[pair_n++] = pair;
    }
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    std::printf("P1_MakeMap input time: %.6f s\n", sec_i);
    std::printf(
        "MtnLineEnds detect time: %.6f s (comps=%u line_tiles=%u ends=%u, %u x %u)\n",
        sec,
        static_cast<u32>(ends.comp_n()),
        static_cast<u32>(line_tile_n),
        static_cast<u32>(pair_n * 2u),
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_ends_viz(out_path, mr.m_terrain, line_ov, w, h, pairs, pair_n)) {
        std::printf("save viz failed\n");
        delete[] line_ov;
        delete[] pairs;
        return -1;
    }
    delete[] line_ov;
    delete[] pairs;
    std::printf("saved: %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    if (!p1_map_gen_init()) {
        std::printf("P1 map generator static init failed\n");
        return -1;
    }
    p1_wb_init(prm.m_w, prm.m_h);
    const i32 rc = test_mtn_line_ends_basic(prm);
    p1_wb_term();
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
