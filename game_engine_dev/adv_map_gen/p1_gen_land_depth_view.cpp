//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_land_depth_view.h"

#include <cstdio>
#include <cstring>

#include "generator_constants.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static const u16 k_inf = 0xFFFFu;
static const u8 k_disp_water[3] = {30, 110, 220};

static inline bool wl_is_water (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_WATER_GRAY;
}

static inline bool wl_is_land (const u8* wl, u32 i) {
    return wl[i] == WL_OVERLAY_LAND_GRAY;
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 wi, u16 hi) {
    if (path == nullptr || rgb == nullptr) {
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
//=> - P1_Gen_LandDepthView -
//================================================================================================================================

bool P1_Gen_LandDepthView::save_pri (cstr path, const u8* wl, const u16* dist, u16 w, u16 h) {
    if (path == nullptr || wl == nullptr || dist == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 max_vis = 0;
    u32 land_fin = 0;
    for (u32 i = 0; i < n; ++i) {
        if (!wl_is_land(wl, i)) {
            continue;
        }
        const u16 d = dist[i];
        if (d == k_inf) {
            continue;
        }
        land_fin++;
        const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
        if (dv > max_vis) {
            max_vis = dv;
        }
    }
    const bool land_all_zero = (max_vis == 0u && land_fin > 0u);
    if (max_vis == 0u) {
        max_vis = 1u;
    }
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8* o = rgb + i * 3u;
        if (wl_is_water(wl, i)) {
            std::memcpy(o, k_disp_water, 3u);
            continue;
        }
        const u16 d = dist[i];
        u8 t = 0;
        if (d != k_inf) {
            if (land_all_zero) {
                t = 255;
            } else {
                const u32 dv = static_cast<u32>(d > 255u ? 255u : static_cast<u32>(d));
                t = static_cast<u8>((dv * 255u) / max_vis);
            }
        }
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
