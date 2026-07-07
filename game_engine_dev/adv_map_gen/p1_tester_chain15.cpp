//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "p1_tester_chain15.h"
#include "p1_adj_ensure_coasts.h"
#include "p1_adj_ensure_seas.h"
#include "p1_adj_ensure_river_valleys.h"
#include "p1_adj_ensure_mtn_foothills.h"
#include "p1_adj_land_altitude.h"
#include "p1_gen_shaped_outline.h"
#include "p1_adj_outline_fill.h"
#include "p1_adj_river_inlets.h"
#include "p1_adj_river_lakes.h"
#include "p1_gen_distance_to_river.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_nearness_to_watershed_mtn.h"
#include "p1_gen_noise_perlin.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_watershed_mountains.h"
#include "p1_gen_watershed_mountain_line_sets.h"
#include "map_terrain_validate.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Private chain helpers -
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
    P1_Gen_RiverSectors sec_gen(prm);
    if (!sec_gen.generate(terrain, w, h, pts_gen.result()) || !sec_gen.is_valid()) {
        return false;
    }
    if (!net_gen->generate(terrain, w, h, sec_gen.result()) || !net_gen->is_valid()) {
        return false;
    }
    if (!lin_gen->generate(terrain, w, h, sec_gen.result(), net_gen->result())
        || !lin_gen->is_valid()) {
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

//================================================================================================================================
//=> - P1 tester chain15 -
//================================================================================================================================

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

bool p1_build_step14_input (
    const P1_RunPrm& prm,
    P1_TesterChain15Rslt* out,
    double* out_sec) 
{
    if (out == nullptr) {
        return false;
    }
    out->m_w = 0;
    out->m_h = 0;
    out->m_terrain = nullptr;
    out->m_river = nullptr;
    out->m_noise = nullptr;
    out->m_dist_riv = nullptr;
    out->m_near_mtn = nullptr;
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    if (terrain == nullptr) {
        return false;
    }
    const clock_t t0i = clock();
    P1_Gen_NoisePerlin noise_gen(prm);
    if (!noise_gen.generate() || !noise_gen.is_valid()) {
        delete[] terrain;
        return false;
    }
    const u8* noise = noise_gen.result().m_ov.data();
    if (noise == nullptr) {
        delete[] terrain;
        return false;
    }
    P1_Gen_RiverLines lin_gen(prm);
    P1_Gen_RiverNetwork net_gen(prm);
    if (!build_step11_input(prm, terrain, w, h, &lin_gen, &net_gen)) {
        delete[] terrain;
        return false;
    }
    P1_Gen_DistanceToRiver dist_gen(prm);
    if (!dist_gen.generate(terrain, w, h, lin_gen.result().m_ov) || !dist_gen.is_valid()) {
        delete[] terrain;
        return false;
    }
    const u8* dist_riv = dist_gen.result().m_ov.data();
    P1_Gen_WatershedMountains border_gen(prm);
    if (!border_gen.generate(terrain, w, h, net_gen.result(), noise) || !border_gen.is_valid()) {
        delete[] terrain;
        return false;
    }
    P1_Gen_WatershedMountainLineSets line_gen(prm);
    if (!line_gen.generate(border_gen.result()) || !line_gen.is_valid()) {
        delete[] terrain;
        return false;
    }
    P1_Gen_NearnessToWatershedMtn near_gen(prm);
    if (!near_gen.generate(terrain, w, h, line_gen.result()) || !near_gen.is_valid()) {
        delete[] terrain;
        return false;
    }
    const u8* near_mtn = near_gen.result().m_ov.data();
    u8* river = new u8[npx];
    if (river == nullptr) {
        delete[] terrain;
        return false;
    }
    std::memcpy(river, lin_gen.result().m_ov, static_cast<size_t>(npx));
    u8* noise_cp = nullptr;
    u8* dist_cp = nullptr;
    u8* near_cp = nullptr;
    if (!copy_ov(&noise_cp, noise, npx)
        || !copy_ov(&dist_cp, dist_riv, npx)
        || !copy_ov(&near_cp, near_mtn, npx)) {
        delete[] near_cp;
        delete[] dist_cp;
        delete[] noise_cp;
        delete[] river;
        delete[] terrain;
        return false;
    }
    const clock_t t1i = clock();
    if (out_sec != nullptr) {
        *out_sec = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    }
    out->m_w = w;
    out->m_h = h;
    out->m_terrain = terrain;
    out->m_river = river;
    out->m_noise = noise_cp;
    out->m_dist_riv = dist_cp;
    out->m_near_mtn = near_cp;
    return true;
}

bool p1_build_step15 (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    P1_TesterChain15Rslt* out,
    double* out_sec) 
{
    if (out == nullptr) {
        return false;
    }
    const clock_t t0i = clock();
    if (!p1_build_step14_input(prm, out, nullptr)) {
        return false;
    }
    P1_Adj_LandAltitude alt_gen(prm, lap);
    if (!alt_gen.adjust(out->m_terrain, out->m_w, out->m_h, out->m_noise, out->m_dist_riv, out->m_near_mtn)
        || !alt_gen.is_valid()) {
        p1_free_chain15(out);
        return false;
    }
    const clock_t t1i = clock();
    if (out_sec != nullptr) {
        *out_sec = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    }
    return true;
}

bool p1_build_ensure_input (
    const P1_RunPrm& prm,
    const P1_Adj_LandAltitudePrm& lap,
    u16 step_n,
    P1_TesterChain15Rslt* out,
    double* out_sec) 
{
    if (out == nullptr || step_n < 18 || step_n > 22) {
        return false;
    }
    const clock_t t0i = clock();
    if (!p1_build_step15(prm, lap, out, nullptr)) {
        return false;
    }
    if (step_n >= 19) {
        P1_Adj_EnsureCoasts coasts(prm);
        if (!coasts.adjust(out->m_terrain, out->m_w, out->m_h) || !coasts.is_valid()) {
            p1_free_chain15(out);
            return false;
        }
    }
    if (step_n >= 20) {
        P1_Adj_EnsureSeas seas(prm);
        if (!seas.adjust(out->m_terrain, out->m_w, out->m_h) || !seas.is_valid()) {
            p1_free_chain15(out);
            return false;
        }
    }
    if (step_n >= 21) {
        P1_Adj_EnsureRiverValleys valleys(prm);
        if (!valleys.adjust(out->m_terrain, out->m_w, out->m_h, out->m_river)
            || !valleys.is_valid()) {
            p1_free_chain15(out);
            return false;
        }
    }
    if (step_n >= 22) {
        P1_Adj_EnsureMtnFoothills foothills(prm);
        if (!foothills.adjust(out->m_terrain, out->m_w, out->m_h) || !foothills.is_valid()) {
            p1_free_chain15(out);
            return false;
        }
    }
    const clock_t t1i = clock();
    if (out_sec != nullptr) {
        *out_sec = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    }
    return true;
}

void p1_free_chain15 (P1_TesterChain15Rslt* out) {
    if (out == nullptr) {
        return;
    }
    delete[] out->m_terrain;
    delete[] out->m_river;
    delete[] out->m_noise;
    delete[] out->m_dist_riv;
    delete[] out->m_near_mtn;
    out->m_terrain = nullptr;
    out->m_river = nullptr;
    out->m_noise = nullptr;
    out->m_dist_riv = nullptr;
    out->m_near_mtn = nullptr;
    out->m_w = 0;
    out->m_h = 0;
}

bool p1_save_terrain_rivers_ppm (
    cstr path,
    const u8* terrain,
    const u8* river,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || river == nullptr || w == 0 || h == 0) {
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
        MapTerrainValidate::rgb_from_class(terrain[i], &r, &g, &b);
        if (river[i] != 0) {
            r = 0;
            g = 0;
            b = 255;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = r;
        rgb[p + 1] = g;
        rgb[p + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
