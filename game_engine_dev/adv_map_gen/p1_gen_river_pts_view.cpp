//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>

#include "p1_gen_river_pts_view.h"
#include "generator_constants.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

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

static void set_px_rgb (u8* rgb, u16 w, u16 h, u16 px, u16 py, u8 r, u8 g, u8 b) {
    if (px >= w || py >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (cls == TERR_OCEAN[0]) {
        *r = TERR_OCEAN[1]; *g = TERR_OCEAN[2]; *b = TERR_OCEAN[3];
    } else if (cls == TERR_SEA[0]) {
        *r = TERR_SEA[1]; *g = TERR_SEA[2]; *b = TERR_SEA[3];
    } else if (cls == TERR_COASTAL[0]) {
        *r = TERR_COASTAL[1]; *g = TERR_COASTAL[2]; *b = TERR_COASTAL[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

//================================================================================================================================
//=> - P1_Gen_RiverPtsView -
//================================================================================================================================

bool P1_Gen_RiverPtsView::save_pri (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const WB_QueXY& que) 
{
    if (path == nullptr || terrain == nullptr || w == 0 || h == 0 || !que.ok()) {
        return false;
    }
    const u32 pt_n = que.count();
    const u32 npx = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(npx) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < npx; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        terr_rgb(terrain[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    for (u32 p = 0; p < pt_n; ++p) {
        const u16 px = que.x_at(p);
        const u16 py = que.y_at(p);
        for (i32 dy = -2; dy <= 2; ++dy) {
            for (i32 dx = -2; dx <= 2; ++dx) {
                if (dx * dx + dy * dy > 4) {
                    continue;
                }
                const i32 x = static_cast<i32>(px) + dx;
                const i32 y = static_cast<i32>(py) + dy;
                if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
                    continue;
                }
                set_px_rgb(rgb, w, h, static_cast<u16>(x), static_cast<u16>(y), 255, 0, 0);
            }
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
