//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "map_terrain_validate.h"
#include "p1_adj_grassland_loess_tiles.h"
#include "p1_gen_loess_boost.h"
#include "p1_make_map.h"
#include "p1_tester_chain_core.h"
#include "p1_tester_util.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static const char* g_terr_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_terrain.ppm";
static const char* g_clim_path = "/home/w/Projects/simple-map-gen/p1-seed-042/24_make_map_climate.ppm";
static const u32 g_seed = 42u;

//================================================================================================================================
//=> - Test helpers -
//================================================================================================================================

static bool load_climate_ppm (cstr path, u16 ew, u16 eh, u8** out_cls) {
    if (path == nullptr || out_cls == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u16 w = 0;
    u16 h = 0;
    if (std::fscanf(fp, "P6\n%u %u\n255\n", (unsigned*)&w, (unsigned*)&h) != 2) {
        std::fclose(fp);
        return false;
    }
    if (w != ew || h != eh) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    u8* cls = new u8[n];
    if (rgb == nullptr || cls == nullptr) {
        delete[] cls;
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    if (std::fread(rgb, 1, nbytes, fp) != nbytes) {
        delete[] cls;
        delete[] rgb;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    for (u32 i = 0; i < n; ++i) {
        bool matched = false;
        cls[i] = climate_from_rgb(rgb[i * 3u + 0], rgb[i * 3u + 1], rgb[i * 3u + 2], &matched);
        if (!matched) {
            delete[] cls;
            delete[] rgb;
            return false;
        }
    }
    delete[] rgb;
    *out_cls = cls;
    return true;
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

static bool save_loess_viz (cstr path, const u8* terrain, const u8* conc, u16 w, u16 h) {
    if (path == nullptr || terrain == nullptr || conc == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 tr = 0;
        u8 tg = 0;
        u8 tb = 0;
        MapTerrainValidate::rgb_from_class(terrain[i], &tr, &tg, &tb);
        const f32 t = static_cast<f32>(conc[i]) / 255.f;
        const f32 a = t * 0.78f;
        const f32 dr = 255.f;
        const f32 dg = 214.f;
        const f32 db = 128.f;
        rgb[i * 3u + 0] = static_cast<u8>(std::lrint(static_cast<f64>(tr) * (1.0 - a) + dr * a));
        rgb[i * 3u + 1] = static_cast<u8>(std::lrint(static_cast<f64>(tg) * (1.0 - a) + dg * a));
        rgb[i * 3u + 2] = static_cast<u8>(std::lrint(static_cast<f64>(tb) * (1.0 - a) + db * a));
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_grass_loess_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* conc,
    u16 w,
    u16 h,
    bool clim_base) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || conc == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 br = 0;
        u8 bg = 0;
        u8 bb = 0;
        if (clim_base) {
            climate_to_rgb(climate[i], &br, &bg, &bb);
        } else {
            MapTerrainValidate::rgb_from_class(terrain[i], &br, &bg, &bb);
        }
        if (climate[i] == CLIMATE_GRASSLAND && conc[i] > 0) {
            const f32 t = static_cast<f32>(conc[i]) / 255.f;
            const f32 a = t * 0.85f;
            const f32 dr = 255.f;
            const f32 dg = 48.f;
            const f32 db = 48.f;
            rgb[i * 3u + 0] = static_cast<u8>(std::lrint(static_cast<f64>(br) * (1.0 - a) + dr * a));
            rgb[i * 3u + 1] = static_cast<u8>(std::lrint(static_cast<f64>(bg) * (1.0 - a) + dg * a));
            rgb[i * 3u + 2] = static_cast<u8>(std::lrint(static_cast<f64>(bb) * (1.0 - a) + db * a));
        } else {
            rgb[i * 3u + 0] = br;
            rgb[i * 3u + 1] = bg;
            rgb[i * 3u + 2] = bb;
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_fertile_adj_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* loess,
    const u8* rain,
    u16 w,
    u16 h,
    const P1_RunPrm& prm) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || loess == nullptr
        || rain == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* terr = new u8[n];
    u8* clim = new u8[n];
    if (terr == nullptr || clim == nullptr) {
        delete[] terr;
        delete[] clim;
        return false;
    }
    std::memcpy(terr, terrain, static_cast<size_t>(n));
    std::memcpy(clim, climate, static_cast<size_t>(n));
    P1_Adj_GrasslandLoessTiles adj(prm);
    if (!adj.adjust(terr, clim, loess, rain, w, h) || !adj.is_valid()) {
        delete[] terr;
        delete[] clim;
        return false;
    }
  u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        delete[] terr;
        delete[] clim;
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        MapTerrainValidate::rgb_from_class(terr[i], &r, &g, &b);
        if (clim[i] != CLIMATE_NONE) {
            climate_to_rgb(clim[i], &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    std::printf("fertile pick: %u (rain_wt=%u)\n", adj.picked_n(), static_cast<u32>(p1_adj_grassland_loess_tiles_prm_def().m_w_rain));
    delete[] rgb;
    delete[] terr;
    delete[] clim;
    return ok;
}

static bool save_desert_viz (cstr path, const u8* climate, u16 w, u16 h) {
    if (path == nullptr || climate == nullptr || w == 0 || h == 0) {
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
        if (climate[i] == CLIMATE_DESERT) {
            climate_to_rgb(CLIMATE_DESERT, &r, &g, &b);
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool make_out (cstr fname, char* out, size_t cap) {
    if (fname == nullptr || out == nullptr || cap == 0) {
        return false;
    }
    char dir[256];
    std::snprintf(dir, sizeof(dir), "%s/p1-seed-%03u", P1_OUT_ROOT, static_cast<unsigned>(g_seed));
    if (!p1_ensure_dir(P1_OUT_ROOT) || !p1_ensure_dir(dir)) {
        return false;
    }
    std::snprintf(out, cap, "%s/%s", dir, fname);
    return true;
}

i32 test_p1_gen_loess_boost_basic (const P1_RunPrm& prm) {
    char conc_path[320];
    char desert_path[320];
    char grass_terr_path[320];
    char grass_clim_path[320];
    char fertile_path[320];
    if (!p1_tester_make_step_out(prm.m_seed, k_p1_step_loess, "loess_boost", conc_path, sizeof(conc_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_loess, "loess_desert_src", desert_path, sizeof(desert_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_loess, "loess_grass_terrain", grass_terr_path, sizeof(grass_terr_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_loess, "loess_grass_climate", grass_clim_path, sizeof(grass_clim_path))
        || !p1_tester_make_step_out(prm.m_seed, k_p1_step_loess, "loess_fertile_5pct", fertile_path, sizeof(fertile_path))) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    P1_MakeMapRslt chain = {};
    double sec_i = 0.0;
    if (!p1_build_chain_core(prm, k_p1_step_climate, &chain, &sec_i)) {
        std::printf("P1 steps 1-24 input failed for step 26\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 h = chain.m_h;
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_rain == nullptr
        || chain.m_wind_dir == nullptr || chain.m_wind_str == nullptr || w == 0 || h == 0) {
        std::printf("invalid chain input for loess\n");
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    P1_Gen_LoessBoost loess(prm);
    const clock_t t0 = clock();
    const bool ok = loess.generate(chain.m_climate, chain.m_wind_dir, chain.m_wind_str, w, h);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !loess.is_valid()) {
        std::printf("P1_Gen_LoessBoost failed to generate\n");
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    const u8* conc = loess.result().m_ov.data();
    if (conc == nullptr) {
        std::printf("P1_Gen_LoessBoost missing overlay\n");
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    std::printf("P1 steps 1-24 input time: %.6f s\n", sec_i);
    std::printf("P1_Gen_LoessBoost generate time: %.6f s (%u x %u)\n",
        sec,
        static_cast<u32>(w),
        static_cast<u32>(h));
    if (!save_loess_viz(conc_path, chain.m_terrain, conc, w, h)) {
        std::printf("failed to save concentration: %s\n", conc_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_desert_viz(desert_path, chain.m_climate, w, h)) {
        std::printf("failed to save desert src: %s\n", desert_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_grass_loess_viz(grass_terr_path, chain.m_terrain, chain.m_climate, conc, w, h, false)) {
        std::printf("failed to save grass terrain: %s\n", grass_terr_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_grass_loess_viz(grass_clim_path, chain.m_terrain, chain.m_climate, conc, w, h, true)) {
        std::printf("failed to save grass climate: %s\n", grass_clim_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    if (!save_fertile_adj_viz(fertile_path, chain.m_terrain, chain.m_climate, conc, chain.m_rain, w, h, prm)) {
        std::printf("failed to save fertile top: %s\n", fertile_path);
        P1_MakeMap::free_rslt(&chain);
        return -1;
    }
    std::printf("saved: %s\n", conc_path);
    std::printf("saved: %s\n", desert_path);
    std::printf("saved: %s\n", grass_terr_path);
    std::printf("saved: %s\n", grass_clim_path);
    std::printf("saved: %s\n", fertile_path);
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
    p1_resolve_run_prm(argc, argv, &prm);
    const i32 rc = test_p1_gen_loess_boost_basic(prm);
    if (!p1_tester_whiteboard_chk()) {
        return -1;
    }
    return rc;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
