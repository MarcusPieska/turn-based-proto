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
#include "p1_gen_river_dist.h"
#include "p1_gen_land_depth.h"
#include "p1_gen_cont_outlines.h"
#include "p1_gen_river_lines.h"
#include "p1_gen_ocean_index.h"
#include "p1_gen_river_network.h"
#include "p1_gen_river_pts.h"
#include "p1_gen_river_sectors.h"
#include "p1_gen_river_sect_adj.h"
#include "p1_gen_coastal_mtn_limits.h"
#include "generator_constants.h"
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

static bool build_step13_input (
    const P1_RunPrm& prm,
    u8* terrain,
    u16 w,
    u16 h,
    u8* riv,
    P1_Gen_RiverSectors* sec_gen,
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
    if (sec_gen == nullptr || net_gen == nullptr) {
        return false;
    }
    P1_Gen_OceanIndex ocn_gen(prm);
    if (!ocn_gen.generate(terrain, w, h) || !ocn_gen.is_valid()) {
        return false;
    }
    if (!sec_gen->generate(terrain, w, h, pts_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !sec_gen->is_valid()) {
        return false;
    }
    P1_Gen_CoastalMtnLimits lim_gen(prm);
    if (!lim_gen.generate(terrain, w, h, sec_gen->result()) || !lim_gen.is_valid()) {
        return false;
    }
    P1_Gen_RiverSectAdj adj_gen(prm);
    if (!adj_gen.generate(sec_gen->result()) || !adj_gen.is_valid()) {
        return false;
    }
    if (!net_gen->generate(terrain, w, h, pts_gen.result(), sec_gen->result(), adj_gen.result(), lim_gen.result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !net_gen->is_valid()) {
        return false;
    }
    P1_Gen_RiverLines lin_gen(prm);
    if (!lin_gen.generate(terrain, w, h, pts_gen.rslt_mut(), sec_gen->result(), net_gen->result(), p1_ocean_ref_from_rslt(ocn_gen.result())) || !lin_gen.is_valid()) {
        return false;
    }
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    std::memcpy(riv, lin_gen.result().m_ov, static_cast<size_t>(npx));
    P1_Adj_CoastalMtnRivers cmr(prm);
    if (!cmr.adjust(terrain, w, h, riv, sec_gen->result(), lim_gen.result()) || !cmr.is_valid()) {
        return false;
    }
    P1_Adj_RiverLakes lakes(prm);
    if (!lakes.adjust(terrain, w, h, riv) || !lakes.is_valid()) {
        return false;
    }
    P1_Adj_RiverInlets inlets(prm);
    if (!inlets.adjust(terrain, w, h, riv, net_gen->result().m_ov) || !inlets.is_valid()) {
        return false;
    }
    return true;
}

static bool is_water (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0]; 
}

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static const i32 k_dx4[4] = {-1, 1, 0, 0};
static const i32 k_dy4[4] = {0, 0, -1, 1};

static u32 tidx (u16 w, u16 x, u16 y) {
    return static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
}

static bool in_map (u16 w, u16 h, i32 x, i32 y) {
    return x >= 0 && y >= 0 && static_cast<u32>(x) < static_cast<u32>(w)
        && static_cast<u32>(y) < static_cast<u32>(h);
}

static u8 norm_gray_seg (u16 d, u16 dmin, u16 dmax) {
    if (d < 1u) {
        return 0;
    }
    if (dmax <= dmin) {
        return 255;
    }
    const u32 span = static_cast<u32>(dmax - dmin);
    const u32 g = (static_cast<u32>(d - dmin) * 255u + span / 2u) / span;
    if (g > 255u) {
        return 255;
    }
    return static_cast<u8>(g);
}

static void paint_riv_red (u8* rgb, u32 i) {
    rgb[i * 3u + 0] = 255;
    rgb[i * 3u + 1] = 0;
    rgb[i * 3u + 2] = 0;
}

static void paint_riv_gray (u8* rgb, u32 i, u8 gv) {
    rgb[i * 3u + 0] = gv;
    rgb[i * 3u + 1] = gv;
    rgb[i * 3u + 2] = gv;
}

static void flood_paint_seg (
    u8* rgb,
    const u8* riv,
    const u16* dist,
    u16 w,
    u16 h,
    u32 seed,
    u8* seg,
    u8* fl,
    u32* q) 
{
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 qn = 0;
    fl[seed] = 1;
    q[qn++] = seed;
    for (u32 qh = 0; qh < qn; ++qh) {
        const u32 i = q[qh];
        const u16 px = static_cast<u16>(i % static_cast<u32>(w));
        const u16 py = static_cast<u16>(i / static_cast<u32>(w));
        for (i32 k = 0; k < 4; ++k) {
            const i32 nx = static_cast<i32>(px) + k_dx4[k];
            const i32 ny = static_cast<i32>(py) + k_dy4[k];
            if (!in_map(w, h, nx, ny)) {
                continue;
            }
            const u32 ni = tidx(w, static_cast<u16>(nx), static_cast<u16>(ny));
            if (fl[ni] != 0 || seg[ni] != 0 || riv[ni] == 0 || dist[ni] < 1u) {
                continue;
            }
            fl[ni] = 1;
            q[qn++] = ni;
            if (qn > n) {
                return;
            }
        }
    }
    u16 dmin = 0xFFFFu;
    u16 dmax = 0;
    for (u32 k = 0; k < qn; ++k) {
        const u32 i = q[k];
        if (dist[i] < dmin) {
            dmin = dist[i];
        }
        if (dist[i] > dmax) {
            dmax = dist[i];
        }
    }
    for (u32 k = 0; k < qn; ++k) {
        const u32 i = q[k];
        seg[i] = 1;
        const u8 gv = norm_gray_seg(dist[i], dmin, dmax);
        paint_riv_gray(rgb, i, gv);
    }
}

static void count_riv_dist_miss (
    const u8* riv,
    const u16* dist,
    u32 n,
    u32* riv_n,
    u32* miss_n) 
{
    u32 rn = 0;
    u32 mn = 0;
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0) {
            continue;
        }
        rn++;
        if (dist[i] < 1u) {
            mn++;
        }
    }
    if (riv_n != nullptr) {
        *riv_n = rn;
    }
    if (miss_n != nullptr) {
        *miss_n = mn;
    }
}

static void print_miss_line (cstr label, u32 missed) {
    if (label == nullptr) {
        return;
    }
    if (missed == 0u) {
        std::printf("%s \033[34mok\033[0m missed=0\n", label);
    } else {
        std::printf("%s \033[31mmissed\033[0m=%u\n", label, missed);
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

static bool save_dist_ppm (
    cstr path,
    const u8* terrain,
    const u8* riv,
    const u16* dist,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || riv == nullptr || dist == nullptr
        || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u8* seg = new u8[n];
    u8* fl = new u8[n];
    u32* q = new u32[n];
    if (rgb == nullptr || seg == nullptr || fl == nullptr || q == nullptr) {
        delete[] q;
        delete[] fl;
        delete[] seg;
        delete[] rgb;
        return false;
    }
    std::memset(seg, 0, n);
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        if (is_water(terrain[i])) {
            b = 255;
        } else if (riv[i] != 0) {
            r = 40;
            g = 40;
            b = 40;
        } else if (is_land(terrain[i])) {
            g = 180;
        } else {
            r = 80;
            g = 80;
            b = 80;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || dist[i] < 1u || seg[i] != 0) {
            continue;
        }
        std::memset(fl, 0, n);
        flood_paint_seg(rgb, riv, dist, w, h, i, seg, fl, q);
    }
    for (u32 i = 0; i < n; ++i) {
        if (riv[i] == 0 || dist[i] >= 1u) {
            continue;
        }
        paint_riv_red(rgb, i);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] q;
    delete[] fl;
    delete[] seg;
    delete[] rgb;
    return ok;
}

i32 test_p1_gen_river_dist_basic (const P1_RunPrm& prm) {
    char up_path[320];
    char dn_path[320];
    if (!p1_tester_make_step_out(prm.m_seed, 163u, "river_dist_up", up_path, sizeof(up_path))
        || !p1_tester_make_step_out(prm.m_seed, 163u, "river_dist_dn", dn_path, sizeof(dn_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    const u16 w = prm.m_w;
    const u16 h = prm.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terrain = new u8[npx];
    u8* riv = new u8[npx];
    if (terrain == nullptr || riv == nullptr) {
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    const clock_t t0i = clock();
    P1_Gen_RiverSectors sec_gen(prm);
    P1_Gen_RiverNetwork net_gen(prm);
    if (!build_step13_input(prm, terrain, w, h, riv, &sec_gen, &net_gen)) {
        std::printf("P1 steps 1-13 input failed for river_dist\n");
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    const clock_t t1i = clock();
    P1_Gen_RiverDist gen(prm);
    const clock_t t0 = clock();
    const bool ok = gen.generate(terrain, w, h, riv, sec_gen.result(), net_gen.result());
    const clock_t t1 = clock();
    const double sec_i = static_cast<double>(t1i - t0i) / static_cast<double>(CLOCKS_PER_SEC);
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_RiverDist failed to generate\n");
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    const P1_Gen_RiverDistRslt& r = gen.result();
    const u16* up = r.m_up.data();
    const u16* dn = r.m_dn.data();
    if (up == nullptr || dn == nullptr) {
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    u32 riv_n = 0;
    u32 miss_up = 0;
    u32 miss_dn = 0;
    count_riv_dist_miss(riv, up, npx, &riv_n, &miss_up);
    count_riv_dist_miss(riv, dn, npx, nullptr, &miss_dn);
    std::printf("P1 steps 1-13 input time: %.6f s\n", sec_i);
    std::printf(
        "P1_Gen_RiverDist generate time: %.6f s (max up %u max dn %u, riv %u, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_max_up),
        static_cast<u32>(r.m_max_dn),
        riv_n,
        static_cast<u32>(w),
        static_cast<u32>(h));
    print_miss_line("river_dist_up", miss_up);
    print_miss_line("river_dist_dn", miss_dn);
    if (!save_dist_ppm(up_path, terrain, riv, up, w, h)) {
        std::printf("failed to save: %s\n", up_path);
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    if (!save_dist_ppm(dn_path, terrain, riv, dn, w, h)) {
        std::printf("failed to save: %s\n", dn_path);
        delete[] riv;
        delete[] terrain;
        return -1;
    }
    delete[] riv;
    delete[] terrain;
    std::printf("saved: %s\n", up_path);
    std::printf("saved: %s\n", dn_path);
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
    return test_p1_gen_river_dist_basic(prm);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
