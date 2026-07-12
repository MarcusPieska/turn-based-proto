//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>
#include <ctime>

#include "game_map_defs.h"
#include "game_primitives.h"
#include "generator_constants.h"
#include "map_terrain_validate.h"
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

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool save_fert_climate_viz (
    cstr path,
    const u8* terrain,
    const u8* climate,
    const u16* fert,
    u16 fert_peak,
    u16 w,
    u16 h) 
{
    if (path == nullptr || terrain == nullptr || climate == nullptr || fert == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    const u32 pk = static_cast<u32>(fert_peak > 0 ? fert_peak : 1u);
    for (u32 i = 0; i < n; ++i) {
        u8 br = 0;
        u8 bg = 0;
        u8 bb = 0;
        climate_to_rgb(climate[i], &br, &bg, &bb);
        if (is_land(terrain[i]) && fert[i] > 0) {
            const f32 t = static_cast<f32>(fert[i]) / static_cast<f32>(pk);
            const f32 a = t * 0.78f;
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


i32 test_p1_gen_rich_coast_fertility_basic (P1_TesterHarness& h) {
    char out_path[320];
    if (!h.path_pri(out_path, sizeof(out_path))) {
        std::printf("failed to ensure output path\n");
        return -1;
    }
    if (!h.run_input()) {
        std::printf("P1 steps 1-23 input failed for rich coast fertility\n");
        return -1;
    }
    const P1_MakeMapRslt& chain = h.mk();
    if (chain.m_terrain == nullptr || chain.m_rain == nullptr || chain.m_w == 0 || chain.m_h == 0) {
        std::printf("invalid chain input for rich coast fertility\n");
        return -1;
    }
    const u16 w = chain.m_w;
    const u16 ht = chain.m_h;
    P1_Gen_Climate clim_gen(h.prm());
    if (!clim_gen.generate(chain.m_terrain, w, ht, chain.m_rivers, chain.m_rain) || !clim_gen.is_valid()) {
        std::printf("P1_Gen_Climate failed for rich coast fertility\n");
        return -1;
    }
    const P1_Gen_RichCoastFertilityPrm fp = p1_gen_rich_coast_fertility_prm_def();
    P1_Gen_RichCoastFertility gen(h.prm(), fp);
    const clock_t t0 = clock();
    const bool ok = gen.generate(chain.m_terrain, w, ht);
    const clock_t t1 = clock();
    const double sec = static_cast<double>(t1 - t0) / static_cast<double>(CLOCKS_PER_SEC);
    if (!ok || !gen.is_valid()) {
        std::printf("P1_Gen_RichCoastFertility failed to generate\n");
        return -1;
    }
    const P1_Gen_RichCoastFertilityRslt& r = gen.result();
    const u16* fert = r.m_ov;
    const u8* climate = clim_gen.result().m_ov.data();
    if (fert == nullptr || climate == nullptr) {
        std::printf("missing fertility or climate data\n");
        return -1;
    }
    u32 land_nz = 0;
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(ht);
    for (u32 i = 0; i < npx; ++i) {
        if (is_land(chain.m_terrain[i]) && fert[i] > 0) {
            land_nz++;
        }
    }
    std::printf("P1 steps 1-23 input time: %.6f s\n", h.input_sec());
    std::printf(
        "P1_Gen_RichCoastFertility gen time: %.6f s (peak %u, %u land tiles, brush %u x %u, rad %u, stamp_lim %u, %u x %u)\n",
        sec,
        static_cast<u32>(r.m_peak),
        land_nz,
        static_cast<u32>(gen.brush_w()),
        static_cast<u32>(gen.brush_h()),
        static_cast<u32>(fp.m_brush_rad),
        static_cast<u32>(fp.m_stamp_lim),
        static_cast<u32>(w),
        static_cast<u32>(ht));
    if (!save_fert_climate_viz(out_path, chain.m_terrain, climate, fert, r.m_peak, w, ht)) {
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
    P1_TesterHarness h;
    if (!h.begin(argc, argv)) {
        return -1;
    }
    const i32 rc = test_p1_gen_rich_coast_fertility_basic(h);
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
