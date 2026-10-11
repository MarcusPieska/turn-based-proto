//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_gen_coastal_land_overlay.h"
#include "p1_gen_coastal_protrusion.h"
#include "p1_gen_shaped_outline_view.h"
#include "p1_proc_distributed_coastal_points.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool is_water_terr (u8 cls) {
    return overlay_is_water_terr(cls);
}

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

static bool save_mark_on_terr (
    cstr path,
    const u8* ter,
    const u8* mark,
    u16 w,
    u16 h,
    u8 mr,
    u8 mg,
    u8 mb,
    u16 px,
    u16 py,
    bool draw_pivot) 
{
    if (path == nullptr || ter == nullptr || mark == nullptr || w == 0 || h == 0) {
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
        if (mark[i] != 0) {
            r = mr;
            g = mg;
            b = mb;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    if (draw_pivot && px < w && py < h) {
        const u32 pi = static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px);
        rgb[pi * 3u + 0] = 255;
        rgb[pi * 3u + 1] = 0;
        rgb[pi * 3u + 2] = 255;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_final_viz (
    cstr path,
    const u8* ter,
    const u8* ov,
    u16 w,
    u16 h) 
{
    if (path == nullptr || ter == nullptr || ov == nullptr || w == 0 || h == 0) {
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
        if (ov[i] != 0) {
            r = 255;
            g = 255;
            b = 255;
        } else if (is_water_terr(ter[i])) {
            r = 40;
            g = 90;
            b = 200;
        } else {
            r = 40;
            g = 140;
            b = 50;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_coastal_protrusion_basic (P1_TesterHarness& h) {
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build step 7 input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_EarlyChainRslt& ec = h.early();
    if (ec.m_ter == nullptr || ec.m_perlin_f32 == nullptr || ec.m_w == 0 || ec.m_h == 0) {
        P1_RPrint::rprint_info("Invalid shaped terrain / perlin from early chain");
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
    P1_Proc_DistributedCoastalPoints pts_proc(h.prm());
    const u32 trips_n = pts_proc.generate(ec.m_ter, ec.m_w, ec.m_h, trips, 16u);
    if (trips_n == 0u) {
        P1_RPrint::rprint_info("Distributed coastal points failed");
        return -1;
    }
    P1_RPrint::rprint_state_u32("P1_Proc_DistributedCoastalPoints", "N", trips_n);

    P1_Gen_CoastalLandOverlay coast_gen(h.prm());
    const clock_t c0 = clock();
    const bool coast_ok = coast_gen.generate(ec.m_ter, ec.m_w, ec.m_h, trips, trips_n);
    const clock_t c1 = clock();
    if (!coast_ok || !coast_gen.is_valid()) {
        P1_RPrint::rprint_info("P1_Gen_CoastalLandOverlay generate failed");
        return -1;
    }
    const P1_Gen_CoastalLandOverlayRslt& cr = coast_gen.result();
    char ctbuf[64];
    std::snprintf(
        ctbuf,
        sizeof(ctbuf),
        "CoastalLand timing: %.6f s",
        static_cast<double>(c1 - c0) / static_cast<double>(CLOCKS_PER_SEC));
    P1_RPrint::rprint_info(ctbuf);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalLandOverlay", "Pts", cr.m_pt_n);
    char coast_path[320];
    if (!p1_make_out_path(h.seed(), "70_coastal_land_overlay.ppm", coast_path, sizeof(coast_path))) {
        P1_RPrint::rprint_info("Failed to ensure coastal overlay path");
        return -1;
    }
    if (!save_final_viz(coast_path, ec.m_ter, cr.m_ov.data(), cr.m_w, cr.m_h)) {
        P1_RPrint::rprint_info("Failed to save coastal overlay");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", coast_path);
    P1_RPrint::rprint_info(obuf);

    P1_Gen_CoastalProtrusion gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(ec.m_ter, ec.m_perlin_f32, ec.m_w, ec.m_h, trips, trips_n);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_info("P1_Gen_CoastalProtrusion generate failed");
        return -1;
    }
    const P1_Gen_CoastalProtrusionRslt& r = gen.result();
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("Map", "W", r.m_w);
    P1_RPrint::rprint_state("Map", "H", r.m_h);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Pts", r.m_pt_n);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Swing", r.m_swing_mode);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Deg", static_cast<u32>(r.m_deg < 0 ? -r.m_deg : r.m_deg));
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "RotMode", r.m_rot_mode);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "PlugMode", r.m_plug_mode);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Src", r.m_src_n);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Rot", r.m_rot_n);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Clip", r.m_clip_n);
    P1_RPrint::rprint_state_u32("P1_Gen_CoastalProtrusion", "Keep", r.m_keep_n);

    char rot_path[320];
    if (!p1_make_out_path(h.seed(), "71_coastal_protrusion_rot.ppm", rot_path, sizeof(rot_path))) {
        P1_RPrint::rprint_info("Failed to ensure rot path");
        return -1;
    }
    if (!save_mark_on_terr(
            rot_path, ec.m_ter, r.m_rot.data(), r.m_w, r.m_h, 255, 220, 40, r.m_px, r.m_py, true)) {
        P1_RPrint::rprint_info("Failed to save rot");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", rot_path);
    P1_RPrint::rprint_info(obuf);

    char clip_path[320];
    if (!p1_make_out_path(h.seed(), "71_coastal_protrusion_clip.ppm", clip_path, sizeof(clip_path))) {
        P1_RPrint::rprint_info("Failed to ensure clip path");
        return -1;
    }
    if (!save_mark_on_terr(
            clip_path, ec.m_ter, r.m_clip.data(), r.m_w, r.m_h, 255, 80, 40, r.m_px, r.m_py, true)) {
        P1_RPrint::rprint_info("Failed to save clip");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", clip_path);
    P1_RPrint::rprint_info(obuf);

    char pri_path[320];
    if (!h.path_pri(pri_path, sizeof(pri_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    if (!save_final_viz(pri_path, ec.m_ter, r.m_ov.data(), r.m_w, r.m_h)) {
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
    const i32 rc = test_p1_gen_coastal_protrusion_basic(h);
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
