//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "generator_constants.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - Internal -
//================================================================================================================================

typedef const char* cstr;

static bool is_start_land (u8 cls) {
    return cls == TERR_PLAINS[0] || cls == TERR_HILLS[0];
}

static void set_px_rgb (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void draw_dot_rgb (u8* rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    set_px_rgb(rgb, w, h, static_cast<i32>(cx), static_cast<i32>(cy), r, g, b);
}

static bool save_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool save_gray_ppm (cstr path, const u8* gray, u16 w, u16 h) {
    if (path == nullptr || gray == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u32 i = 0; i < n; ++i) {
        const u8 g = gray[i];
        rgb[i * 3u + 0] = g;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = g;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_pts_ppm (cstr path, const u8* terr, u16 w, u16 h, const SpgPt* pts, u32 n) {
    if (path == nullptr || terr == nullptr || w == 0 || h == 0 || pts == nullptr) {
        return false;
    }
    const u32 px_n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(px_n) * 3u];
    for (u32 i = 0; i < px_n; ++i) {
        u8 r = 255;
        u8 g = 255;
        u8 b = 255;
        if (is_start_land(terr[i])) {
            r = 128;
            g = 128;
            b = 128;
        }
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 i = 0; i < n; ++i) {
        draw_dot_rgb(rgb, w, h, static_cast<u16>(pts[i].x), static_cast<u16>(pts[i].y), 0, 0, 0);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

static bool save_mk3_outputs (cstr out_dir, const StartingPointGenerator& gen) {
    if (out_dir == nullptr) {
        return false;
    }
    const u16 w = gen.map_width();
    const u16 h = gen.map_height();
    const u8* terr = gen.terrain();
    const u8* gray = gen.dist_gray();
    if (terr == nullptr || gray == nullptr) {
        return false;
    }
    char path[512];
    std::snprintf(path, sizeof(path), "%s/DEL_mk3_00_water_dist.ppm", out_dir);
    if (!save_gray_ppm(path, gray, w, h)) {
        return false;
    }
    std::snprintf(path, sizeof(path), "%s/DEL_mk3_01_prob_cand.ppm", out_dir);
    if (!save_pts_ppm(path, terr, w, h, gen.candidates(), gen.candidate_count())) {
        return false;
    }
    std::snprintf(path, sizeof(path), "%s/DEL_mk3_02_pick.ppm", out_dir);
    if (!save_pts_ppm(path, terr, w, h, gen.picks(), gen.pick_count())) {
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
