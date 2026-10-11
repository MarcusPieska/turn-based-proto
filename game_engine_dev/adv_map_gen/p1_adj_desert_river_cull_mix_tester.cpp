//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_adj_desert_river_cull_mix.h"
#include "p1_map_summary_view.h"
#include "p1_rprint.h"
#include "p1_tester_harness.h"
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

static bool save_mark_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u8* mark,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || mark == nullptr
        || w == 0 || h == 0) {
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
        if (terrain[i] == TERR_MOUNTAINS[0]) {
            r = TERR_MOUNTAINS[1];
            g = TERR_MOUNTAINS[2];
            b = TERR_MOUNTAINS[3];
        }
        if (mark[i] != 0) {
            r = 255;
            g = 0;
            b = 0;
        }
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

i32 test_p1_adj_desert_river_cull_mix_basic (P1_TesterHarness& h) {
    char sum_path[320];
    char harsh_path[320];
    if (!h.path_pri(sum_path, sizeof(sum_path)) || !h.path_sec(harsh_path, sizeof(harsh_path))) {
        P1_RPrint::rprint_info("Failed to ensure output paths");
        return -1;
    }
    if (!h.run_input()) {
        P1_RPrint::rprint_info("Failed to build climate-chain input");
        return -1;
    }
    char ibuf[64];
    std::snprintf(ibuf, sizeof(ibuf), "Input: %.6f s", h.input_sec());
    P1_RPrint::rprint_info(ibuf);
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_climate == nullptr || chain.m_rivers == nullptr
        || chain.m_flags == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        P1_RPrint::rprint_info("Invalid chain input");
        return -1;
    }
    const u32 npx = static_cast<u32>(chain.m_w) * static_cast<u32>(chain.m_h);
    u8* riv = new u8[npx];
    if (riv == nullptr) {
        return -1;
    }
    std::memcpy(riv, chain.m_rivers, static_cast<size_t>(npx));
    P1_Adj_DesertRiverCullMix adj(h.prm());
    const clock_t t0 = clock();
    const bool ok = adj.adjust(
        riv, chain.m_w, chain.m_h, chain.m_terrain, chain.m_climate, chain.m_flags);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        delete[] riv;
        P1_RPrint::rprint_info("P1_Adj_DesertRiverCullMix adjust failed");
        return -1;
    }
    const P1_Adj_DesertRiverCullMixRslt& r = adj.result();
    char tbuf[64];
    std::snprintf(tbuf, sizeof(tbuf), "Timing: %.6f s", sec);
    P1_RPrint::rprint_info(tbuf);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "Sys", r.m_sys_n);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "HarshSys", r.m_harsh_n);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "MildSys", r.m_mild_n);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "HarshTiles", r.m_harsh_tiles);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "MildTiles", r.m_mild_tiles);
    P1_RPrint::rprint_state_u32("P1_Adj_DesertRiverCullMix", "Culled", r.m_culled_n);
    if (!P1_MapSummaryView::save(
            sum_path, chain.m_terrain, chain.m_climate, riv, chain.m_w, chain.m_h)) {
        delete[] riv;
        P1_RPrint::rprint_info("Failed to save summary");
        return -1;
    }
    char obuf[384];
    std::snprintf(obuf, sizeof(obuf), "Output: %s", sum_path);
    P1_RPrint::rprint_info(obuf);
    if (!save_mark_viz(
            harsh_path, chain.m_terrain, chain.m_climate, r.m_harsh.data(), chain.m_w, chain.m_h)) {
        delete[] riv;
        P1_RPrint::rprint_info("Failed to save harsh systems viz");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", harsh_path);
    P1_RPrint::rprint_info(obuf);
    char mild_path[320];
    if (!p1_make_out_path(h.seed(), "74_desert_river_cull_mix_mild.ppm", mild_path, sizeof(mild_path))) {
        delete[] riv;
        P1_RPrint::rprint_info("Failed to ensure mild systems path");
        return -1;
    }
    if (!save_mark_viz(
            mild_path, chain.m_terrain, chain.m_climate, r.m_mild.data(), chain.m_w, chain.m_h)) {
        delete[] riv;
        P1_RPrint::rprint_info("Failed to save mild systems viz");
        return -1;
    }
    std::snprintf(obuf, sizeof(obuf), "Output: %s", mild_path);
    P1_RPrint::rprint_info(obuf);
    delete[] riv;
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
    const i32 rc = test_p1_adj_desert_river_cull_mix_basic(h);
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
