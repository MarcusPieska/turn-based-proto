//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_nearness_to_watershed_mtn.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "game_primitives.h"
#include "p1_tester_util.h"

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

static bool copy_ov (u8** dst, const u8* src, u32 n) {
    if (dst == nullptr || src == nullptr || n == 0) {
        return false;
    }
    delete[] *dst;
    *dst = new u8[n];
    if (*dst == nullptr) {
        return false;
    }
    std::memcpy(*dst, src, static_cast<size_t>(n));
    return true;
}

static bool coast_lim_rslt_from_ov (
    u16 w,
    u16 h,
    const u8* coast_lim,
    P1_Gen_CoastalMtnLimitsRslt* out) 
{
    if (coast_lim == nullptr || out == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (!out->m_limit_ov.resize(w, h)) {
        return false;
    }
    u8* dst = out->m_limit_ov.data_w();
    if (dst == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(dst, coast_lim, static_cast<size_t>(n));
    out->m_w = w;
    out->m_h = h;
    return true;
}

static bool build_step11_input (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    P1_Gen_RiverLines* lin_gen,
    P1_Gen_RiverNetwork* net_gen,
    u8** coast_lim_out) 
{
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        return false;
    }
    if (ov_map.width() != w || ov_map.height() != h) {
        return false;
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr) {
        return false;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        return false;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        return false;
    }
    if (!build_step5_terrain(prm, terrain, w, h, ov, land_depth)) {
        return false;
    }
    P1_Gen_RiverPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h) || !pts_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result()) || !sec_gen.is_valid()) {
        return false;
    }
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    if (!lim_gen.generate(terrain, w, h, sec_gen.result()) || !lim_gen.is_valid()) {
        return false;
    }
    if (coast_lim_out != nullptr) {
        const u8* lim = lim_gen.result().m_limit_ov.data();
        const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
        if (lim == nullptr || !copy_ov(coast_lim_out, lim, npx)) {
            return false;
        }
    }
    if (!net_gen->generate(terrain, w, h, sec_gen.result(), lim_gen.result()) || !net_gen->is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, sec_gen.result(), net_gen->result())
        || !lin_gen->is_valid()) {
        return false;
    }
    P1_Adj_CoastalMtnRivers cmr(prm);
    if (!cmr.adjust(terrain, w, h, lin_gen->result().m_ov, sec_gen.result(), lim_gen.result())
        || !cmr.is_valid()) {
        return false;
    }
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, lin_gen->result().m_ov) || !lakes.is_valid()) {
        return false;
    }
    P1_Adj_RiverInlets inlets(prm);
    return inlets.adjust(terrain, w, h, lin_gen->result().m_ov, lin_gen->result())
        && inlets.is_valid();
}

i32 test_p1_gen_nearness_to_watershed_mtn_basic (const P1_RunPrm& prm) {
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
    const clock_t t0i = clock();
    P1_Gen_NoisePerlin noise_gen(prm);
    if (!noise_gen.generate() || !noise_gen.is_valid()) {
        std::printf("P1_Gen_NoisePerlin failed for step 16 input\n");
        delete[] terrain;
        return -1;
    }
    const u8* noise = noise_gen.result().m_ov.data();
    if (noise == nullptr) {
        std::printf("invalid perlin noise data\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverLines lin_gen(prm);
    P1_Gen_RiverNetwork net_gen(prm);
    u8* coast_lim = nullptr;
    if (!build_step11_input(prm, terrain, w, h, &lin_gen, &net_gen, &coast_lim)) {
        std::printf("P1 steps 1-13 input failed for step 16\n");
        delete[] coast_lim;
        delete[] terrain;
        return -1;
    }
    P1_Gen_WatershedMountains border_gen(prm);
    if (!border_gen.generate(terrain, w, h, net_gen.result(), noise) || !border_gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountains failed for step 16 input\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_WatershedMountainLineSets line_gen(prm);
    if (!line_gen.generate(border_gen.result()) || !line_gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountainLineSets failed for step 16 input\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_NearnessToWatershedMtn gen(prm);
    P1_Gen_CoastalMtnLimitsRslt coast_r;
    if (!coast_lim_rslt_from_ov(w, h, coast_lim, &coast_r)) {
        std::printf("failed to build coastal mtn input for step 16\n");
        delete[] coast_lim;
        delete[] terrain;
        return -1;
    }
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, h, line_gen.result(), coast_r);
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_NearnessToWatershedMtn failed to generate\n");
        delete[] coast_lim;
        delete[] terrain;
        return -1;
    }
    const P1_Gen_NearnessToWatershedMtnRslt& r = gen.result();
    u32 land_nz = 0;
    const u8* pix = r.m_ov.data();
    if (pix != nullptr) {
        for (u32 i = 0; i < npx; ++i) {
            if (pix[i] != 0) {
                land_nz++;
            }
        }
    }
    std::printf("P1 steps 1-14 input time: %.6f s\n", sec_i);
    std::printf(
        "P1_Gen_NearnessToWatershedMtn generate time: %.6f s (max dist %u, %u land tiles, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_max_dist),
        land_nz,
        static_cast<u32>(w),
        static_cast<u32>(h));
    gen.save_output(out_path);
    delete[] coast_lim;
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
    return test_p1_gen_nearness_to_watershed_mtn_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
