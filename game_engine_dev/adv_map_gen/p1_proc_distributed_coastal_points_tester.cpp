//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_gen_coastal_land_overlay.h"
#include "p1_gen_shaped_outline_view.h"
#include "p1_proc_distributed_coastal_points.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static const i32 k_dot_r = 5;

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

static void paint_dot (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, i32 rad, u8 r, u8 g, u8 b) {
    if (rgb == nullptr || rad < 0) {
        return;
    }
    const i32 r2 = rad * rad;
    for (i32 dy = -rad; dy <= rad; ++dy) {
        for (i32 dx = -rad; dx <= rad; ++dx) {
            if (dx * dx + dy * dy > r2) {
                continue;
            }
            const i32 x = static_cast<i32>(cx) + dx;
            const i32 y = static_cast<i32>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
                continue;
            }
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
}

static bool save_trips_viz (
    cstr path,
    const u8* ter,
    u16 w,
    u16 h,
    const P1_CoastPtTrip* trips,
    u32 trips_n) 
{
    if (path == nullptr || ter == nullptr || trips == nullptr || w == 0 || h == 0) {
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
        MapTerrainValidate::rgb_from_class(ter[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 k = 0; k < trips_n; ++k) {
        paint_dot(rgb, w, h, trips[k].m_a.m_x, trips[k].m_a.m_y, k_dot_r, 255, 220, 40);
        paint_dot(rgb, w, h, trips[k].m_b.m_x, trips[k].m_b.m_y, k_dot_r, 255, 220, 40);
    }
    for (u32 k = 0; k < trips_n; ++k) {
        paint_dot(rgb, w, h, trips[k].m_cen.m_x, trips[k].m_cen.m_y, k_dot_r, 255, 0, 0);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_proc_distributed_coastal_points_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 7 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || ec.m_w == 0 || ec.m_h == 0) {
        P1_RPrint::rprint_info("Invalid shaped terrain from early chain");
        return -1;
    }
    char merge_path[320];
    if (!p1_make_out_path(h.seed(), "07_shaped_outline_merge.ppm", merge_path, sizeof(merge_path))) {
        P1_RPrint::rprint_info("Failed to ensure merge output path");
        return -1;
    }
    if (!P1_Gen_ShapedOutlineView::save_pri(merge_path, ec.m_ter, ec.m_w, ec.m_h)) {
        P1_RPrint::rprint_info("Failed to save shaped outline merge");
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", merge_path);
    P1_RPrint::rprint_info(obuf);

    P1_CoastPtTrip trips[16];
    P1_Proc_DistributedCoastalPoints proc(h.prm());
    const clock_t t0 = clock();
    const u32 n = proc.generate(ec.m_ter, ec.m_w, ec.m_h, trips, 16u);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (n == 0u) {
        P1_RPrint::rprint_info("P1_Proc_DistributedCoastalPoints found no points");
        return -1;
    }
    P1_Gen_CoastalLandOverlay flood(h.prm());
    if (!flood.generate(ec.m_ter, ec.m_w, ec.m_h, trips, n) || !flood.is_valid()) {
        P1_RPrint::rprint_info("Coastal flood failed (stalls)");
        return -1;
    }
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Proc_DistributedCoastalPoints", "N", n);
    for (u32 i = 0; i < n; ++i) {
        char pbuf[160];
        std::snprintf(
            pbuf,
            sizeof(pbuf),
            "Trip%u: cen(%u,%u) a(%u,%u) b(%u,%u)",
            static_cast<unsigned>(i),
            static_cast<unsigned>(trips[i].m_cen.m_x),
            static_cast<unsigned>(trips[i].m_cen.m_y),
            static_cast<unsigned>(trips[i].m_a.m_x),
            static_cast<unsigned>(trips[i].m_a.m_y),
            static_cast<unsigned>(trips[i].m_b.m_x),
            static_cast<unsigned>(trips[i].m_b.m_y));
        P1_RPrint::rprint_info(pbuf);
    }

    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!save_trips_viz(pri_path, ec.m_ter, ec.m_w, ec.m_h, trips, n)) {
        P1_RPrint::rprint_info("Failed to save primary output");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", pri_path);
    P1_RPrint::rprint_info(obuf);
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
    const i32 rc = test_p1_proc_distributed_coastal_points_basic(h);
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
