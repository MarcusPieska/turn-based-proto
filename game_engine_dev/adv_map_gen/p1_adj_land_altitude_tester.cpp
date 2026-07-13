//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_adj_land_altitude.h"
#include "generator_constants.h"
#include "perlin_noise.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_shaped_outline.h"
#include "p1_gen_river_prob.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_dynamic_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_distance_to_river.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "p1_gen_nearness_to_watershed_mtn.h"
#include "p1_tester_util.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private viz helpers -
//================================================================================================================================

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
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

static bool save_terrain_ppm (cstr path, const u8* terrain, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0) {
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

static i32 test_p1_adj_land_altitude_basic (const P1_RunPrm& prm, const P1_Adj_LandAltitudePrm& sp) {
    char out_path[320];
    char joint_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    if (!p1_make_out_path(prm.m_seed, p1_tester_out_sec(), joint_path, sizeof(joint_path))) {
        std::printf("failed to build joint output path\n");
        return -1;
    }
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        std::printf("P1 step01 outline failed\n");
        return -1;
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr) {
        return -1;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        return -1;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        return -1;
    }
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        delete[] terrain;
        return -1;
    }
    const P1_MapConfig cfg = p1_map_config_def();
    const P1_Gen_NoisePerlinPrm nprm = p1_gen_noise_perlin_prm_from_cfg(cfg, w, h);
    f32* perlin_f32 = new f32[static_cast<size_t>(npx)];
    if (perlin_f32 == nullptr) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_NoisePerlin noise_gen(prm);
    if (!noise_gen.generate_with_combo(prm.m_seed, nprm, perlin_f32) || !noise_gen.is_valid()) {
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    const u8* noise = noise_gen.result().m_ov.data();
    if (noise == nullptr) {
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    const P1_Gen_ShapedOutlinePrm sh_prm = p1_gen_shaped_outline_prm_from_cfg(cfg);
    u8* near_ter = new u8[npx];
    u8* far_ter = new u8[npx];
    if (near_ter == nullptr || far_ter == nullptr) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    std::memcpy(near_ter, terrain, static_cast<size_t>(npx));
    std::memcpy(far_ter, terrain, static_cast<size_t>(npx));
    P1_Gen_ShapedOutline shaped(prm);
    shaped.bind_perlin_field(perlin_f32, w, h);
    if (!shaped.generate_layer(near_ter, w, h, ov, land_depth, sh_prm.m_radial_near, sh_prm.m_shelf_near) || !shaped.is_valid()) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    if (!shaped.generate_layer(far_ter, w, h, ov, land_depth, sh_prm.m_radial_far, sh_prm.m_shelf_far) || !shaped.is_valid()) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    if (!shaped.merge_layers(terrain, w, h, ov, land_depth, near_ter, far_ter) || !shaped.is_valid()) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        delete[] terrain;
        return -1;
    }
    delete[] far_ter;
    delete[] near_ter;
    delete[] perlin_f32;
    P1_Gen_RiverProb prob_gen(prm);
    if (!prob_gen.generate(terrain, w, h) || !prob_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    const P1_OceanIndexRef ocn_ref = p1_ocean_ref_from_rslt(ocn_gen.result());
    P1_Gen_RiverDynamicPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h, prob_gen.result(), ocn_ref) || !pts_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result(), ocn_ref) || !sec_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen.result()) || !adj_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_CoastalMtnLimits coast_lim(prm);
    if (!coast_lim.generate(terrain, w, h, sec_gen.result(), adj_gen.result()) || !coast_lim.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverNetwork net_gen(prm);
    if (!net_gen.generate(terrain, w, h, pts_gen.result(), sec_gen.result(), adj_gen.result(), coast_lim.result(), ocn_ref) || !net_gen.is_valid()) {
        delete[] terrain;
        return -1;
    }
    P1_Gen_RiverLines lin_gen(prm);
    if (!lin_gen.generate(terrain, w, h, const_cast<P1_Gen_RiverPtsRslt&>(pts_gen.result()), sec_gen.result(), net_gen.result(), ocn_ref) || !lin_gen.is_valid()) {
        std::printf("P1_Gen_RiverLines failed\n");
        delete[] terrain;
        return -1;
    }
    u8* river = new u8[npx];
    if (river == nullptr) {
        std::printf("alloc river failed\n");
        delete[] terrain;
        return -1;
    }
    std::memcpy(river, lin_gen.result().m_ov, static_cast<size_t>(npx));
    P1_Adj_CoastalMtnRivers cmr(prm);
    if (!cmr.adjust(terrain, w, h, river, sec_gen.result(), coast_lim.result()) || !cmr.is_valid()) {
        std::printf("P1_Adj_CoastalMtnRivers failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, river) || !lakes.is_valid()) {
        std::printf("P1_Adj_RiverLakes failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Adj_RiverInlets inlets(prm);
    if (!inlets.adjust(terrain, w, h, river, net_gen.result().m_ov) || !inlets.is_valid()) {
        std::printf("P1_Adj_RiverInlets failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Gen_DistanceToRiver dist_gen(prm);
    if (!dist_gen.generate(terrain, w, h, river) || !dist_gen.is_valid()) {
        std::printf("P1_Gen_DistanceToRiver failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    const u8* dist_riv = dist_gen.result().m_ov.data();
    if (dist_riv == nullptr) {
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Gen_WatershedMountains wshed_gen(prm);
    if (!wshed_gen.generate(terrain, w, h, net_gen.result(), pts_gen.result(), noise) || !wshed_gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountains failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Gen_WatershedMountainLineSets line_gen(prm);
    if (!line_gen.generate(wshed_gen.result()) || !line_gen.is_valid()) {
        std::printf("P1_Gen_WatershedMountainLineSets failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    P1_Gen_NearnessToWatershedMtn near_gen(prm);
    if (!near_gen.generate(terrain, w, h, line_gen.result(), coast_lim.result()) || !near_gen.is_valid()) {
        std::printf("P1_Gen_NearnessToWatershedMtn failed\n");
        delete[] river;
        delete[] terrain;
        return -1;
    }
    const u8* near_mtn = near_gen.result().m_ov.data();
    if (near_mtn == nullptr) {
        delete[] river;
        delete[] terrain;
        return -1;
    }
    u8* terrain_pre = new u8[npx];
    if (terrain_pre == nullptr) {
        delete[] river;
        delete[] terrain;
        return -1;
    }
    std::memcpy(terrain_pre, terrain, static_cast<size_t>(npx));
    std::printf(
        "land altitude prm: w_noise %.3f w_near %.3f w_riv %.3f lim_hills %.3f lim_mtn %.3f\n",
        sp.m_w_noise,
        sp.m_w_near,
        sp.m_w_riv,
        sp.m_lim_hills,
        sp.m_lim_mtn);
    P1_Adj_LandAltitude alt(prm, sp);
    const clock_t t0 = clock();
    const bool ok = alt.adjust(terrain, w, h, noise, dist_riv, near_mtn);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !alt.is_valid()) {
        std::printf("P1_Adj_LandAltitude failed to adjust\n");
        delete[] terrain_pre;
        delete[] river;
        delete[] terrain;
        return -1;
    }
    u8* joint = new u8[npx];
    if (joint == nullptr) {
        delete[] terrain_pre;
        delete[] river;
        delete[] terrain;
        return -1;
    }
    if (!alt.joint_ov(terrain_pre, w, h, noise, dist_riv, near_mtn, joint)) {
        std::printf("P1_Adj_LandAltitude joint_ov failed\n");
        delete[] joint;
        delete[] terrain_pre;
        delete[] river;
        delete[] terrain;
        return -1;
    }
    std::printf("P1_Adj_LandAltitude adjust time: %.6f s (%u x %u)\n", sec, static_cast<u32>(w), static_cast<u32>(h));
    if (!save_terrain_ppm(out_path, terrain, w, h)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] joint;
        delete[] terrain_pre;
        delete[] river;
        delete[] terrain;
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    if (!save_perlin_gray_pgm(joint_path, joint, w, h)) {
        std::printf("failed to save joint: %s\n", joint_path);
        delete[] joint;
        delete[] terrain_pre;
        delete[] river;
        delete[] terrain;
        return -1;
    }
    std::printf("saved: %s\n", joint_path);
    delete[] joint;
    delete[] terrain_pre;
    delete[] river;
    delete[] terrain;
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
    P1_Adj_LandAltitudePrm sp;
    p1_resolve_land_altitude_prm(argc, argv, &sp);
    const i32 rc = test_p1_adj_land_altitude_basic(prm, sp);
    const bool wb_ok = p1_tester_whiteboard_chk();
    p1_wb_term();
    return (rc == 0 && wb_ok) ? 0 : -1;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
