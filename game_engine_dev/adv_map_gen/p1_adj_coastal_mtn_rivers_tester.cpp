//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "generator_constants.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool build_step5_terrain (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    const u8* ov,
    const u16* land_depth) {
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        return false;
    }
    const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_def();
    return p1_apply_shaped_outline(prm, sp, terrain, w, h, ov, land_depth);
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (cls == TERR_OCEAN[0]) {
        *r = 0;
        *g = 0;
        *b = 128;
    } else if (cls == TERR_SEA[0]) {
        *r = 0;
        *g = 128;
        *b = 255;
    } else if (cls == TERR_COASTAL[0]) {
        *r = 0;
        *g = 200;
        *b = 255;
    } else if (cls == TERR_PLAINS[0]) {
        *r = 0;
        *g = 180;
        *b = 0;
    } else if (cls == TERR_HILLS[0]) {
        *r = 120;
        *g = 100;
        *b = 0;
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = 180;
        *g = 180;
        *b = 180;
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = 0;
        *g = 100;
        *b = 200;
    } else {
        *r = 80;
        *g = 80;
        *b = 80;
    }
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

static bool save_riv_ppm (cstr path, const u8* terrain, const u8* riv, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || riv == nullptr || w == 0 || h == 0) {
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
        terr_rgb(terrain[i], &r, &g, &b);
        if (riv[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_adj_coastal_mtn_rivers_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map) || ov_map.width() != w || ov_map.height() != h) {
        delete[] terrain;
        return -1;
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr || !build_step5_terrain(prm, terrain, w, h, ov, land_depth)) {
        delete[] terrain;
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_RiverPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h) || !pts_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !sec_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen.result()) || !adj_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    if (!lim_gen.generate(terrain, w, h, sec_gen.result(), adj_gen.result()) || !lim_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverNetwork net_gen(prm);
    if (!net_gen.generate(terrain, w, h, pts_gen.result(), sec_gen.result(), adj_gen.result(), lim_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !net_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverLines lin_gen(prm);
    if (!lin_gen.generate(terrain, w, h, const_cast<P1_Gen_RiverPtsRslt&>(pts_gen.result()), sec_gen.result(), net_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !lin_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    u8* riv = new u8[npx];
    if (riv == nullptr) {
        delete[] terrain;
        return -1;
    }
    std::memcpy(riv, lin_gen.result().m_ov, static_cast<size_t>(npx));
    const clock_t t1i = clock();
    P1_Adj_CoastalMtnRivers adj(prm);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, w, h, riv, sec_gen.result(), lim_gen.result());
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_CoastalMtnRivers failed to adjust\n");
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    u32 riv_n = 0;
    for (u32 i = 0; i < npx; ++i) {
        if (riv[i] != 0) {
            riv_n++;
        }
    }
    std::printf("P1 steps 1-12 input time: %.6f s\n", sec_i);
    std::printf("P1_Adj_CoastalMtnRivers adjust time: %.6f s (%u x %u) paths=%u riv_tiles=%u\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h),
        adj.path_n(),
        riv_n);
    if (!save_riv_ppm(out_path, terrain, riv, w, h)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    delete[] riv;
    delete[] terrain;
    std::printf("saved: %s\n", out_path);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

i32 main (i32 argc, char* argv[]) {
    if (!p1_tester_checkout(argc, argv)) {
        return -1;
    }
    P1_RunPrm prm;
    p1_resolve_run_prm(argc, argv, &prm);
    p1_wb_init(prm.m_w, prm.m_h);
    const i32 rc = test_p1_adj_coastal_mtn_rivers_basic(prm);
    const bool wb_ok = p1_tester_whiteboard_chk();
    p1_wb_term();
    return (rc == 0 && wb_ok) ? 0 : -1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
