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
#include "p1_gen_cont_outlines.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "map_terrain_data.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool build_step5_terrain (const P1_RunPrm& prm, u8* terrain, u16 w, u16 h, const u8* ov, const u16* land_depth) {
    P1_Adj_OutlineFill fill(prm);
    if (!fill.adjust(terrain, w, h, ov) || !fill.is_valid()) {
        return false;
    }
    const P1_Gen_ShapedOutlinePrm sp = p1_gen_shaped_outline_prm_def();
    return p1_apply_shaped_outline(prm, sp, terrain, w, h, ov, land_depth);
}

static bool build_step9_input (const P1_RunPrm& prm, u8* terrain, u16 w, u16 h, P1_Gen_RiverLines* lin_gen, u16* basin) {
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
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !sec_gen.is_valid()) {
        return false;
    }
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    if (!lim_gen.generate(terrain, w, h, sec_gen.result()) || !lim_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen.result()) || !adj_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverNetwork net_gen(prm);
    if (!net_gen.generate(terrain, w, h, pts_gen.result(), sec_gen.result(), adj_gen.result(), lim_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !net_gen.is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, pts_gen.rslt_mut(), sec_gen.result(), net_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result()))
        || !lin_gen->is_valid()) {
        return false;
    }
    if (basin != nullptr) {
        const u16* b = net_gen.result().m_ov;
        const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
        if (b == nullptr) {
            return false;
        }
        std::memcpy(basin, b, static_cast<size_t>(npx) * sizeof(u16));
    }
    P1_Adj_CoastalMtnRivers cmr(prm);
    return cmr.adjust(terrain, w, h, lin_gen->result().m_ov, sec_gen.result(), lim_gen.result())
        && cmr.is_valid();
}

i32 test_p1_adj_river_inlets_basic (const P1_RunPrm& prm) {
    char out_path[320];
    if (!p1_tester_make_out(prm.m_seed, out_path, sizeof(out_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    u16* basin = new u16[npx];
    if (terrain == nullptr || basin == nullptr) {
        delete[] basin;
        delete[] terrain;
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_RiverLines lin_gen(prm);
    if (!build_step9_input(prm, terrain, w, h, &lin_gen, basin)) {
        std::printf("P1 steps 1-11 input failed for step 13\n");
        delete[] basin;
        delete[] terrain;
        return -1;
    }
    const P1_Gen_RiverLinesRslt& lines = lin_gen.result();
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, lines.m_ov) || !lakes.is_valid()) {
        std::printf("P1_Adj_RiverLakes failed for step 13 input\n");
        delete[] basin;
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Adj_RiverInlets adj(prm);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terrain, w, h, lines.m_ov, basin);
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_RiverInlets failed to adjust\n");
        delete[] basin;
        delete[] terrain;
        return -1;
    }
    std::printf("P1 steps 1-12 input time: %.6f s\n", sec_i);
    std::printf("P1_Adj_RiverInlets adjust time: %.6f s (%u x %u) perc=%u min=%u\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h),
        static_cast<u32>(P1_RIVER_INLET_PERC_DEF),
        static_cast<u32>(P1_RIVER_INLET_MIN_DEF));
    MapTerrainData map;
    if (!map.assign_raw(w, h, terrain)) {
        std::printf("failed to assign terrain\n");
        delete[] basin;
        delete[] terrain;
        return -1;
    }
    delete[] basin;
    delete[] terrain;
    if (!map.save_terrain_ppm(out_path)) {
        std::printf("failed to save map: %s\n", out_path);
        return -1;
    }
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
    return test_p1_adj_river_inlets_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
