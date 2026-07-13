//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>

#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "game_primitives.h"
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

static bool build_step11_input (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    P1_Gen_RiverPtsRslt* pts_out,
    P1_Gen_RiverLines* lin_gen,
    P1_Gen_RiverNetwork* net_gen) 
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
    if (pts_out == nullptr) {
        return false;
    }
    pts_out->m_w = w;
    pts_out->m_h = h;
    pts_out->m_n = pts_gen.result().m_n;
    pts_out->m_ocn_sec_n = pts_gen.result().m_ocn_sec_n;
    pts_out->m_que.clear();
    for (u32 pi = 0; pi < pts_gen.result().m_n; ++pi) {
        if (!pts_out->m_que.push(pts_gen.result().m_que.x_at(pi), pts_gen.result().m_que.y_at(pi))) {
            return false;
        }
    }
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !sec_gen.is_valid()) {
        return false;
    }
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen.result()) || !adj_gen.is_valid()) {
        return false;
    }
    if (!lim_gen.generate(terrain, w, h, sec_gen.result(), adj_gen.result()) || !lim_gen.is_valid()) {
        return false;
    }
    if (!net_gen->generate(terrain, w, h, pts_gen.result(), sec_gen.result(), adj_gen.result(), lim_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !net_gen->is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, pts_gen.rslt_mut(), sec_gen.result(), net_gen->result(), p1_ocean_ref_from_rslt(ocn_gen.result()))
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
    return inlets.adjust(terrain, w, h, lin_gen->result().m_ov, net_gen->result().m_ov)
        && inlets.is_valid();
}

i32 test_p1_gen_watershed_mountain_line_sets_basic (const P1_RunPrm& prm) {
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
        std::printf("P1_Gen_NoisePerlin failed for step 14 input\n");
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
    P1_Gen_RiverPtsRslt pts_rslt;
    if (!build_step11_input(prm, terrain, w, h, &pts_rslt, &lin_gen, &net_gen)) {
        std::printf("P1 steps 1-13 input failed for line sets\n");
        delete[] terrain;
        return -1;
    }
    P1_Gen_WatershedMountains border_gen(prm);
    if (!border_gen.generate(terrain, w, h, net_gen.result(), pts_rslt, noise) || !border_gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountains failed for line sets input\n");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_WatershedMountainLineSets gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(border_gen.result());
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountainLineSets failed to generate\n");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_WatershedMountainLineSetsRslt& r = gen.result();
    const P1_Gen_WatershedMountainsRslt& br = border_gen.result();
    std::printf("P1 steps 1-14 input time: %.6f s\n", sec_i);
    std::printf(
        "P1_Gen_WatershedMountainLineSets generate time: %.6f s (%u/%u segments picked, %u/%u border tiles, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_pick_n),
        static_cast<u32>(br.m_seg_n),
        static_cast<u32>(r.m_pick_tile_n),
        static_cast<u32>(br.m_border_tile_n),
        static_cast<u32>(w),
        static_cast<u32>(h));
    for (u16 pi = 0; pi < r.m_pick_n; ++pi) {
        const u16 si = r.m_pick_seg[pi];
        const P1_WatershedBorderSeg& s = br.m_segs[si];
        std::printf(
            "  pick %u seg %u basins %u-%u dist %u tiles %u\n",
            pi,
            si,
            s.m_basin_a,
            s.m_basin_b,
            s.m_mouth_dist,
            s.m_tile_n);
    }
    gen.save_output(out_path, net_gen.result(), terrain);
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
    const i32 rc = test_p1_gen_watershed_mountain_line_sets_basic(prm);
    p1_wb_term();
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
