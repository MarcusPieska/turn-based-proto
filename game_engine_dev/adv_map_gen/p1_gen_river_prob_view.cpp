//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_prob_view.h"

#include <cstdio>
#include <cstring>

#include "generator_constants.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static const u8 k_disp_wat[3] = {30, 110, 220};
static const u32 k_band_n = 10u;

static bool is_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static bool is_land (u8 cls) {
    return !is_wat(cls) && cls != TERR_MOUNTAINS[0];
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)wi, (unsigned)hi);
    const size_t nbytes = static_cast<size_t>(wi) * static_cast<size_t>(hi) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool save_u16_gray (cstr path, u16 w, u16 h, const u16* src) {
    if (path == nullptr || src == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 vmax = 0u;
    for (u32 i = 0; i < n; ++i) {
        const u32 v = static_cast<u32>(src[i]);
        if (v > vmax) {
            vmax = v;
        }
    }
    if (vmax == 0u) {
        vmax = 1u;
    }
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        const u8 t = static_cast<u8>((static_cast<u32>(src[i]) * 255u) / vmax);
        u8* o = rgb + i * 3u;
        o[0] = t;
        o[1] = t;
        o[2] = t;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static u8 band_gray (u8 band) {
    if (band >= k_band_n - 1u) {
        return 255u;
    }
    return static_cast<u8>((static_cast<u32>(band) * 255u) / (k_band_n - 1u));
}

//================================================================================================================================
//=> - P1_Gen_RiverProbView -
//================================================================================================================================

bool P1_Gen_RiverProbView::save_wat (cstr path, u16 w, u16 h, const u16* wat_dist) {
    return save_u16_gray(path, w, h, wat_dist);
}

bool P1_Gen_RiverProbView::save_eq (cstr path, u16 w, u16 h, const u16* eq_near) {
    return save_u16_gray(path, w, h, eq_near);
}

bool P1_Gen_RiverProbView::save_wgt (cstr path, u16 w, u16 h, const u16* wgt_sum) {
    return save_u16_gray(path, w, h, wgt_sum);
}

bool P1_Gen_RiverProbView::save_pri (cstr path, const u8* terrain, u16 w, u16 h, const u8* band) {
    if (path == nullptr || terrain == nullptr || band == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8* o = rgb + i * 3u;
        if (is_wat(terrain[i])) {
            std::memcpy(o, k_disp_wat, 3u);
            continue;
        }
        if (!is_land(terrain[i])) {
            o[0] = 0u;
            o[1] = 0u;
            o[2] = 0u;
            continue;
        }
        const u8 t = band_gray(band[i]);
        o[0] = t;
        o[1] = t;
        o[2] = t;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
