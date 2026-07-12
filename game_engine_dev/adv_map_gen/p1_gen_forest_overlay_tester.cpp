//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_gen_forest_overlay.h"
#include "p1_tester_harness.h"
#include "res_placement_defs.h"

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

static bool save_res_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* river,
    const u8* res_ov,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || res_ov == nullptr || w == 0 || h == 0) {
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
        climate_to_rgb(climate[i], &br, &bg, &bb);
        if (res_ov[i] == static_cast<u8>(RES_OV_FORESTS)) {
            br = 0;
            bg = 100;
            bb = 0;
        } else if (res_ov[i] == static_cast<u8>(RES_OV_SWAMPS)) {
            br = 255;
            bg = 0;
            bb = 0;
        } else if (res_ov[i] == static_cast<u8>(RES_OV_JUNGLES)) {
            br = 0;
            bg = 160;
            bb = 40;
        }
        if (terrain[i] == TERR_MOUNTAINS[0]) {
            MapTerrainValidate::rgb_from_class(TERR_MOUNTAINS[0], &br, &bg, &bb);
        }
        if (river != nullptr && river[i] != 0) {
            br = 0;
            bg = 0;
            bb = 255;
        }
        rgb[i * 3u + 0] = br;
        rgb[i * 3u + 1] = bg;
        rgb[i * 3u + 2] = bb;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_gen_forest_overlay_basic (P1_TesterHarness& h) {
    char out_path[320];
    char dist_path[320];
    char dist_des_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-32 input failed for forest overlay\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for forest overlay\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(ht);
    u8* res_ov = new u8[n];
    if (res_ov == nullptr) {
        return -1;
    }
    std::memset(res_ov, 0, static_cast<size_t>(n));
    P1_Gen_ForestOverlay gen(h.prm());
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, chain.m_climate, res_ov, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_ForestOverlay failed to generate\n");
        delete[] res_ov;
        return -1;
    }
    const P1_Gen_ForestOverlayRslt& r = gen.result();
    std::printf("P1 steps 1-32 input time: %.6f s\n", h.input_sec());
    std::printf("P1_Gen_ForestOverlay gen time: %.6f s (forest %u, %u x %u)\n",
        sec, r.m_forest_n, static_cast<u32>(w), static_cast<u32>(ht));
    if (h.path_extra("forest_overlay_dist", dist_path, sizeof(dist_path))
        && h.path_extra("forest_overlay_desert_dist", dist_des_path, sizeof(dist_des_path))) {
        gen.save_dist_output(dist_path);
        std::printf("saved: %s\n", dist_path);
        gen.save_desert_dist_output(dist_des_path);
        std::printf("saved: %s\n", dist_des_path);
    }
    if (!save_res_climate_viz(out_path, chain.m_terrain, chain.m_climate, chain.m_rivers, res_ov, w, ht)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] res_ov;
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    delete[] res_ov;
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
    const i32 rc = test_p1_gen_forest_overlay_basic(h);
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
