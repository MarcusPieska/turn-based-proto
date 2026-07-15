//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_adj_coastal_mtn_rivers.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_dynamic_pts.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_prob.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_shaped_outline.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_make_map.h"
#include "map_config.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool copy_pts_rslt (const P1_Gen_RiverPtsRslt& src, P1_Gen_RiverPtsRslt* out) {
    if (out == nullptr || src.m_n == 0u || !src.m_que.ok()) {
        return false;
    }
    out->m_w = src.m_w;
    out->m_h = src.m_h;
    out->m_n = src.m_n;
    out->m_ocn_sec_n = src.m_ocn_sec_n;
    out->m_que.clear();
    for (u32 pi = 0; pi < src.m_n; ++pi) {
        if (!out->m_que.push(src.m_que.x_at(pi), src.m_que.y_at(pi))) {
            return false;
        }
    }
    return out->m_que.ok();
}

static bool build_mk_watershed_input (
    const P1_RunPrm& prm,
    const P1_MakeMapPrm& mp,
    u8* terrain,
    u16 w,
    u16 h,
    const u8** noise_out,
    P1_Gen_RiverPtsRslt* pts_out,
    P1_Gen_NoisePerlin* noise_gen,
    P1_Gen_RiverNetwork* net_gen,
    P1_Gen_RiverLines* lin_gen) 
{
    if (terrain == nullptr || noise_out == nullptr || pts_out == nullptr
        || noise_gen == nullptr || net_gen == nullptr || lin_gen == nullptr) {
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    const MapConfig cfg = map_config_def();
    const P1_Gen_NoisePerlinPrm nprm = p1_gen_noise_perlin_prm_from_cfg(cfg, w, h);
    f32* perlin_f32 = new f32[npx];
    if (perlin_f32 == nullptr) {
        return false;
    }
    if (!noise_gen->generate_with_combo(prm.m_seed, nprm, perlin_f32) || !noise_gen->is_valid()) {
        delete[] perlin_f32;
        return false;
    }
    const u8* noise = noise_gen->result().m_ov.data();
    if (noise == nullptr) {
        delete[] perlin_f32;
        return false;
    }
    MapArrayOverlay ov_map;
    if (!p1_gen_step01_ov(prm, &ov_map)) {
        delete[] perlin_f32;
        return false;
    }
    const u8* ov = ov_map.data();
    if (ov == nullptr || ov_map.width() != w || ov_map.height() != h) {
        delete[] perlin_f32;
        return false;
    }
    P1_Gen_LandDepth depth_gen(prm);
    if (!depth_gen.generate(ov, w, h) || !depth_gen.is_valid()) {
        delete[] perlin_f32;
        return false;
    }
    const u16* land_depth = depth_gen.result().m_dist.data();
    if (land_depth == nullptr) {
        delete[] perlin_f32;
        return false;
    }
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        delete[] perlin_f32;
        return false;
    }
    u8* near_ter = new u8[npx];
    u8* far_ter = new u8[npx];
    if (near_ter == nullptr || far_ter == nullptr) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        return false;
    }
    std::memcpy(near_ter, terrain, static_cast<size_t>(npx));
    std::memcpy(far_ter, terrain, static_cast<size_t>(npx));
    P1_Gen_ShapedOutline shaped(prm);
    shaped.bind_perlin_field(perlin_f32, w, h);
    if (!shaped.generate_layer(near_ter, w, h, ov, land_depth, mp.m_shaped.m_radial_near, mp.m_shaped.m_shelf_near)
        || !shaped.is_valid()
        || !shaped.generate_layer(far_ter, w, h, ov, land_depth, mp.m_shaped.m_radial_far, mp.m_shaped.m_shelf_far)
        || !shaped.is_valid()
        || !shaped.merge_layers(terrain, w, h, ov, land_depth, near_ter, far_ter)
        || !shaped.is_valid()) {
        delete[] far_ter;
        delete[] near_ter;
        delete[] perlin_f32;
        return false;
    }
    delete[] far_ter;
    delete[] near_ter;
    delete[] perlin_f32;
    P1_Gen_RiverProb prob_gen(prm);
    if (!prob_gen.generate(terrain, w, h) || !prob_gen.is_valid()) {
        return false;
    }
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        return false;
    }
    const P1_OceanIndexRef ocn_ref = p1_ocean_ref_from_rslt(ocn_gen.result());
    P1_Gen_RiverDynamicPts pts_gen(prm);
    if (!pts_gen.generate(terrain, w, h, prob_gen.result(), ocn_ref) || !pts_gen.is_valid()) {
        return false;
    }
    if (!copy_pts_rslt(pts_gen.result(), pts_out)) {
        return false;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result(), ocn_ref) || !sec_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen.result()) || !adj_gen.is_valid()) {
        return false;
    }
    P1_Gen_CoastalMtnLimits coast_lim(prm);
    if (!coast_lim.generate(terrain, w, h, sec_gen.result(), adj_gen.result()) || !coast_lim.is_valid()) {
        return false;
    }
    if (!net_gen->generate(terrain, w, h, pts_gen.result(), sec_gen.result(), adj_gen.result(), coast_lim.result(), ocn_ref)
        || !net_gen->is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, const_cast<P1_Gen_RiverPtsRslt&>(pts_gen.result()), sec_gen.result(), net_gen->result(), ocn_ref)
        || !lin_gen->is_valid()) {
        return false;
    }
    P1_Adj_CoastalMtnRivers cmr(prm);
    if (!cmr.adjust(terrain, w, h, lin_gen->result().m_ov, sec_gen.result(), coast_lim.result())
        || !cmr.is_valid()) {
        return false;
    }
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, lin_gen->result().m_ov) || !lakes.is_valid()) {
        return false;
    }
    P1_Adj_RiverInlets inlets(prm);
    if (!inlets.adjust(terrain, w, h, lin_gen->result().m_ov, net_gen->result().m_ov) || !inlets.is_valid()) {
        return false;
    }
    *noise_out = noise;
    return true;
}

//================================================================================================================================
//=> - Test body -
//================================================================================================================================

i32 test_p1_gen_watershed_mountains_basic (P1_TesterHarness& h) {
    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        P1_RPrint::rprint_info("Failed to ensure primary output path");
        return -1;
    }
    const P1_RunPrm& prm = h.prm();
    const P1_MakeMapPrm mp = p1_make_map_prm_def();
    const u16 w = prm.m_w;
    const u16 hgt = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(hgt);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_NoisePerlin noise_gen(prm);
    P1_Gen_RiverLines lin_gen(prm);
    P1_Gen_RiverNetwork net_gen(prm);
    P1_Gen_RiverPtsRslt pts_rslt;
    const u8* noise = nullptr;
    if (!build_mk_watershed_input(prm, mp, terrain, w, hgt, &noise, &pts_rslt, &noise_gen, &net_gen, &lin_gen)) {
        P1_RPrint::rprint_info("P1_MakeMap steps 1-18 input failed for watershed mountains");
        delete[] terrain;
        return -1;
    }
    if (noise == nullptr) {
        P1_RPrint::rprint_info("Invalid perlin noise data");
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_WatershedMountains gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, hgt, net_gen.result(), pts_rslt, noise);
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        P1_RPrint::rprint_info("P1_Gen_WatershedMountains failed to generate");
        delete[] terrain;
        return -1;
    }
    const P1_Gen_WatershedMountainsRslt& r = gen.result();
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", sec_i);
    P1_RPrint::rprint_info(ibuf);
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state("P1_Gen_WatershedMountains", "Segments", r.m_seg_n);
    P1_RPrint::rprint_state_u32("P1_Gen_WatershedMountains", "BorderTiles", r.m_border_tile_n);
    P1_RPrint::rprint_state("Map", "W", w);
    P1_RPrint::rprint_state("Map", "H", hgt);
    gen.save_output(out_path, net_gen.result(), terrain);
    delete[] terrain;
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", out_path);
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
    const i32 rc = test_p1_gen_watershed_mountains_basic(h);
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
