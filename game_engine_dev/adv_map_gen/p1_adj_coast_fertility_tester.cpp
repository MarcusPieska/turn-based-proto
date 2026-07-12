//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <ctime>
#include <cstring>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "map_terrain_validate.h"
#include "p1_adj_coast_fertility.h"
#include "p1_gen_climate.h"
#include "p1_gen_rich_coast_fertility.h"
#include "p1_tester_harness.h"

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

static void count_climate (
    const u8* climate,
    u32 n,
    u32* grass,
    u32* plains,
    u32* desert,
    u32* black,
    u32* none) 
{
    *grass = 0;
    *plains = 0;
    *desert = 0;
    *black = 0;
    *none = 0;
    for (u32 i = 0; i < n; ++i) {
        const u8 c = climate[i];
        if (c == CLIMATE_GRASSLAND) {
            ++(*grass);
        } else if (c == CLIMATE_PLAINS) {
            ++(*plains);
        } else if (c == CLIMATE_DESERT) {
            ++(*desert);
        } else if (c == CLIMATE_BLACK_SOIL) {
            ++(*black);
        } else {
            ++(*none);
        }
    }
}

static void print_climate_stats (cstr label, const u8* climate, u32 n) {
    u32 grass = 0;
    u32 plains = 0;
    u32 desert = 0;
    u32 black = 0;
    u32 none = 0;
    count_climate(climate, n, &grass, &plains, &desert, &black, &none);
    std::printf(
        "%s climate: grass %u plains %u desert %u black %u none %u\n",
        label,
        grass,
        plains,
        desert,
        black,
        none);
}

static bool save_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
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
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

i32 test_p1_adj_coast_fertility_basic (P1_TesterHarness& h) {
    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-23 input failed for coast fertility adjust\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_rain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for coast fertility adjust\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    P1_Gen_Climate clim_gen(h.prm());
    if (!clim_gen.generate(chain.m_terrain, w, ht, chain.m_rivers, chain.m_rain) || !clim_gen.is_valid()) {
        std::printf("P1_Gen_Climate failed for coast fertility adjust\n");
        return -1;
    }
    const u8* base_clim = clim_gen.result().m_ov.data();
    if (base_clim == nullptr) {
        std::printf("missing base climate data\n");
        return -1;
    }
    u8* climate = new u8[npx];
    if (climate == nullptr) {
        return -1;
    }
    std::memcpy(climate, base_clim, static_cast<size_t>(npx));
    u32 adj_grass = 0;
    u32 adj_plains = 0;
    u32 adj_nz = 0;
  {
    const P1_Gen_RichCoastFertilityPrm fp = p1_gen_rich_coast_fertility_prm_def();
    P1_Gen_RichCoastFertility fert_gen(h.prm(), fp);
    if (!fert_gen.generate(chain.m_terrain, w, ht) || !fert_gen.is_valid()) {
        std::printf("P1_Gen_RichCoastFertility failed for coast fertility adjust\n");
        delete[] climate;
        return -1;
    }
    const u16* fert = fert_gen.result().m_ov;
    if (fert == nullptr) {
        std::printf("missing fertility overlay\n");
        delete[] climate;
        return -1;
    }
    const P1_Adj_CoastFertilityPrm ap = p1_adj_coast_fertility_prm_def();
    P1_Adj_CoastFertility adj(h.prm(), ap);
    const clock_t t0 = clock();
    const bool ok = adj.adjust(climate, w, ht, fert);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !adj.is_valid()) {
        std::printf("P1_Adj_CoastFertility failed to adjust\n");
        delete[] climate;
        return -1;
    }
    adj_nz = adj.nz_n();
    adj_grass = adj.grass_n();
    adj_plains = adj.plains_n();
    std::printf("P1 steps 1-23 input time: %.6f s\n", h.input_sec());
    std::printf(
        "P1_Adj_CoastFertility adjust time: %.6f s (nz %u, elevated grass %u, elevated plains %u, grass_pct %u, plains_pct %u)\n",
        sec,
        adj_nz,
        adj_grass,
        adj_plains,
        static_cast<u32>(ap.m_grass_pct),
        static_cast<u32>(ap.m_plains_pct));
  }
    print_climate_stats("before", base_clim, npx);
    print_climate_stats("after", climate, npx);
    if (!save_climate_viz(out_path, chain.m_terrain, climate, w, ht)) {
        std::printf("failed to save map: %s\n", out_path);
        delete[] climate;
        return -1;
    }
    std::printf("saved: %s\n", out_path);
    delete[] climate;
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
    const i32 rc = test_p1_adj_coast_fertility_basic(h);
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
