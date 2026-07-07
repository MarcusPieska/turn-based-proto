//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_adj_grassland_loess_tiles.h"
#include "p1_make_map.h"
#include "p1_tester_chain_core.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

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

static bool save_terrain_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* river,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || w == 0 || h == 0) {
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
        const u8 cl = climate[i];
        if (cl != CLIMATE_NONE) {
            climate_to_rgb(cl, &r, &g, &b);
        }
        if (river != nullptr && river[i] != 0) {
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

static bool run_rain_wt (
    const P1_RunPrm& prm,
    const P1_MakeMapRslt& chain,
    u8 rain_wt) 
{
    char out_path[320];
    char def_path[320];
    char suffix[64];
    std::snprintf(suffix, sizeof(suffix), "grassland_loess_rain_wt_%02u", static_cast<unsigned>(rain_wt));
    if (!p1_tester_make_step_out(prm.m_seed, k_p1_step_grass_loess, suffix, out_path, sizeof(out_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_grass_loess, "grassland_loess_tiles", def_path, sizeof(def_path))) {
        std::printf("failed to ensure output path rain_wt=%u\n", static_cast<unsigned>(rain_wt));
        return false;
    }
    const u16 w = chain.m_w;
    const u16 h = chain.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terr = new u8[n];
    u8* clim = new u8[n];
    if (terr == nullptr || clim == nullptr) {
        delete[] terr;
        delete[] clim;
        return false;
    }
    std::memcpy(terr, chain.m_terrain, static_cast<size_t>(n));
    std::memcpy(clim, chain.m_climate, static_cast<size_t>(n));
    P1_Adj_GrasslandLoessTilesPrm sp = p1_adj_grassland_loess_tiles_prm_def();
    sp.m_w_rain = static_cast<u16>(rain_wt);
    P1_Adj_GrasslandLoessTiles adj(prm, sp);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(terr, clim, chain.m_loess, chain.m_rain, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_GrasslandLoessTiles failed rain_wt=%u\n", static_cast<unsigned>(rain_wt));
        delete[] terr;
        delete[] clim;
        return false;
    }
    std::printf("P1_Adj_GrasslandLoessTiles rain_wt=%3u pick=%u time: %.6f s (%u x %u)\n",
        static_cast<unsigned>(rain_wt),
        adj.picked_n(),
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_terrain_climate_viz(out_path, terr, clim, chain.m_rivers, w, h)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] terr;
        delete[] clim;
        return false;
    }
    std::printf("saved: %s\n", out_path);
    if (rain_wt == p1_adj_grassland_loess_tiles_prm_def().m_w_rain
        && !save_terrain_climate_viz(def_path, terr, clim, chain.m_rivers, w, h)) {
        std::printf("failed to save default map: %s\n", def_path);
        delete[] terr;
        delete[] clim;
        return false;
    }
    if (rain_wt == p1_adj_grassland_loess_tiles_prm_def().m_w_rain) {
        std::printf("saved: %s\n", def_path);
    }
    delete[] terr;
    delete[] clim;
    return true;
}

i32 test_p1_adj_grassland_loess_tiles_basic (const P1_RunPrm& prm, u8 rain_wt_arg, bool rain_wt_set) {
    P1_MakeMapRslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_chain_core(prm, k_p1_step_loess, &chain, &sec_i)) {
        std::printf("P1 steps 1-26 input failed for step 27\n");
        return -1;
    }
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_loess == nullptr
        || chain.m_rain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for grassland loess\n");
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    std::printf("P1 steps 1-26 input time: %.6f s\n", sec_i);
    if (rain_wt_set) {
        if (!run_rain_wt(prm, chain, rain_wt_arg)) {
            P1_MakeMap::free_rslt(&chain);
            return -1;
        }
    } else {
        for (u32 wi = 0; wi <= 10u; ++wi) {
            u8 wt = static_cast<u8>(wi * 10u);
            if (wt > static_cast<u8>(CLIMATE_WT_MAX)) {
                wt = static_cast<u8>(CLIMATE_WT_MAX);
            }
            if (!run_rain_wt(prm, chain, wt)) {
                P1_MakeMap::free_rslt(&chain);
                return -1;
            }
        }
    }
    P1_MakeMap::free_rslt(&chain);
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
    u8 rain_wt = 0;
    bool rain_wt_set = false;
    p1_resolve_run_prm(argc, argv, &prm);
    p1_resolve_climate_rain_wt(argc, argv, &rain_wt, &rain_wt_set);
    const i32 rc = test_p1_adj_grassland_loess_tiles_basic(prm, rain_wt, rain_wt_set);
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
