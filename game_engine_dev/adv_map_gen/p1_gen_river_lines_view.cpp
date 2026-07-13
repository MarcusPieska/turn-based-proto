//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_lines_view.h"

#include <cstdio>

#include "generator_constants.h"
#include "p1_gen_river_network.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

static const u8 k_dov_none = 0xFFu;

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

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    *r = 0;
    *g = 0;
    *b = 0;
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

static u8 dist_gray (u8 d, u8 dmax) {
    if (d == k_dov_none) {
        return 0u;
    }
    if (dmax <= 1u) {
        return 255u;
    }
    const u32 span = static_cast<u32>(dmax - 1u);
    const u32 g = 255u - (static_cast<u32>(d) * 255u + span / 2u) / span;
    return static_cast<u8>(g > 255u ? 255u : g);
}

static void draw_px (u8* rgb, u16 w, u16 h, i32 cx, i32 cy, u8 r, u8 g, u8 b) {
    if (cx < 0 || cy < 0 || static_cast<u32>(cx) >= static_cast<u32>(w) || static_cast<u32>(cy) >= static_cast<u32>(h)) {
        return;
    }
    const u32 p = (static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx)) * 3u;
    rgb[p + 0] = r;
    rgb[p + 1] = g;
    rgb[p + 2] = b;
}

//================================================================================================================================
//=> - P1_Gen_RiverLinesView -
//================================================================================================================================

bool P1_Gen_RiverLinesView::save_pri (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverLinesRslt& r,
    const P1_Gen_RiverNetworkRslt& network,
    const P1_Gen_RiverPtsRslt& pts) {
    if (path == nullptr || terrain == nullptr || r.m_ov == nullptr || w == 0 || h == 0) {
        return false;
    }
    if (network.m_downstream == nullptr || !pts.m_que.ok()) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        terr_rgb(terrain[i], &cr, &cg, &cb);
        if (r.m_ov[i] != 0) {
            cr = 0;
            cg = 0;
            cb = 255;
        }
        rgb[i * 3u + 0u] = cr;
        rgb[i * 3u + 1u] = cg;
        rgb[i * 3u + 2u] = cb;
    }
    for (u32 si = 0; si < static_cast<u32>(network.m_sector_n); ++si) {
        if (network.m_downstream[si] != static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
            continue;
        }
        if (si >= pts.m_n) {
            continue;
        }
        draw_px(
            rgb,
            w,
            h,
            static_cast<i32>(pts.m_que.x_at(si)),
            static_cast<i32>(pts.m_que.y_at(si)),
            255,
            0,
            0);
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool P1_Gen_RiverLinesView::save_dist (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    const P1_Gen_RiverLinesRslt& r) {
    if (path == nullptr || terrain == nullptr || r.m_dov == nullptr || w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8 dmax = 1u;
    for (u32 i = 0; i < n; ++i) {
        const u8 d = r.m_dov[i];
        if (d != k_dov_none && d > dmax) {
            dmax = d;
        }
    }
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        terr_rgb(terrain[i], &cr, &cg, &cb);
        const u8 d = r.m_dov[i];
        if (d != k_dov_none) {
            const u8 gv = dist_gray(d, dmax);
            cr = gv;
            cg = gv;
            cb = gv;
        }
        rgb[i * 3u + 0u] = cr;
        rgb[i * 3u + 1u] = cg;
        rgb[i * 3u + 2u] = cb;
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
