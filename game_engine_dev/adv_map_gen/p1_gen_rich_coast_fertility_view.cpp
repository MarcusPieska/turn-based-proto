//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cmath>
#include <cstdio>

#include "p1_gen_rich_coast_fertility_view.h"

#include "game_map_defs.h"
#include "p1_wb_util.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static bool is_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0] || cls == TERR_MOUNTAINS[0];
}

static bool wr_rgb_ppm (cstr path, const u8* r, const u8* g, const u8* b, u16 wi, u16 hi) {
    if (path == nullptr || r == nullptr || g == nullptr || b == nullptr || wi == 0 || hi == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    u8 row[4096 * 3];
    for (u16 y = 0; y < hi; ++y) {
        for (u16 x = 0; x < wi; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(wi) + static_cast<u32>(x);
            row[static_cast<size_t>(x) * 3u + 0u] = r[i];
            row[static_cast<size_t>(x) * 3u + 1u] = g[i];
            row[static_cast<size_t>(x) * 3u + 2u] = b[i];
        }
        if (std::fwrite(row, 1, static_cast<size_t>(wi) * 3u, fp) != static_cast<size_t>(wi) * 3u) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

static bool fill_pri_rgb (
    u8* rr,
    u8* gg,
    u8* bb,
    const u8* terrain,
    const u8* climate,
    const u16* fert,
    u16 fert_peak,
    u32 n) 
{
    if (rr == nullptr || gg == nullptr || bb == nullptr || terrain == nullptr || climate == nullptr || fert == nullptr) {
        return false;
    }
    const u32 pk = static_cast<u32>(fert_peak > 0 ? fert_peak : 1u);
    for (u32 i = 0; i < n; ++i) {
        u8 br = 0;
        u8 bg = 0;
        u8 bb0 = 0;
        climate_to_rgb(climate[i], &br, &bg, &bb0);
        if (is_land(terrain[i]) && fert[i] > 0) {
            const f32 t = static_cast<f32>(fert[i]) / static_cast<f32>(pk);
            const f32 a = t * 0.78f;
            rr[i] = static_cast<u8>(std::lrint(static_cast<f64>(br) * (1.0 - a) + 255.0 * a));
            gg[i] = static_cast<u8>(std::lrint(static_cast<f64>(bg) * (1.0 - a) + 48.0 * a));
            bb[i] = static_cast<u8>(std::lrint(static_cast<f64>(bb0) * (1.0 - a) + 48.0 * a));
        } else {
            rr[i] = br;
            gg[i] = bg;
            bb[i] = bb0;
        }
    }
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RichCoastFertilityView -
//================================================================================================================================

bool P1_Gen_RichCoastFertilityView::save_pri (
    cstr path,
    u32 seed,
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
    Whiteboard_1B wb_r("P1_Gen_RichCoastFertilityView", "rgb_r", seed);
    Whiteboard_1B wb_g("P1_Gen_RichCoastFertilityView", "rgb_g", seed);
    Whiteboard_1B wb_b("P1_Gen_RichCoastFertilityView", "rgb_b", seed);
    P1_WB_CHK(wb_r);
    P1_WB_CHK(wb_g);
    P1_WB_CHK(wb_b);
    u8* rr = wb_r.raw();
    u8* gg = wb_g.raw();
    u8* bb = wb_b.raw();
    if (rr == nullptr || gg == nullptr || bb == nullptr) {
        return false;
    }
    if (!fill_pri_rgb(rr, gg, bb, terrain, climate, fert, fert_peak, n)) {
        return false;
    }
    return wr_rgb_ppm(path, rr, gg, bb, w, h);
}

bool P1_Gen_RichCoastFertilityView::save_fert_gray (
    cstr path,
    const u16* ov,
    u16 w,
    u16 h,
    u16 peak) 
{
    if (path == nullptr || ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P5\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const u32 pk = static_cast<u32>(peak > 0 ? peak : 1u);
    u8 row[4096];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            row[x] = static_cast<u8>((static_cast<u32>(ov[i]) * 255u) / pk);
        }
        if (std::fwrite(row, 1, static_cast<size_t>(w), fp) != static_cast<size_t>(w)) {
            std::fclose(fp);
            return false;
        }
    }
    std::fclose(fp);
    return true;
}

bool P1_Gen_RichCoastFertilityView::save_brush_extra (
    cstr path,
    const u16* brush,
    u16 brush_w,
    u16 brush_h,
    u16 brush_peak) 
{
    if (path == nullptr || brush == nullptr || brush_w == 0 || brush_h == 0 || brush_peak == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(brush_w) * static_cast<u32>(brush_h);
    const u32 pk = static_cast<u32>(brush_peak);
    u8 gray[65u * 65u];
    if (n > sizeof(gray)) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        gray[i] = static_cast<u8>((static_cast<u32>(brush[i]) * 255u) / pk);
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P5\n%u %u\n255\n", (unsigned)brush_w, (unsigned)brush_h);
    if (std::fwrite(gray, 1, static_cast<size_t>(n), fp) != static_cast<size_t>(n)) {
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    return true;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
