//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_network_view.h"

#include <cstdio>

#include "generator_constants.h"
#include "p1_gen_river_network.h"

//================================================================================================================================
//=> - Private view helpers -
//================================================================================================================================

struct Rng32 {
    u32 m_s;
};

static void rng_seed (Rng32* g, u32 seed) {
    g->m_s = seed != 0u ? seed : 1u;
}

static u32 rng_next (Rng32* g) {
    g->m_s = g->m_s * 1664525u + 1013904223u;
    return g->m_s;
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

static void draw_thick_line (
    u8* rgb,
    u16 w,
    u16 h,
    i32 x1,
    i32 y1,
    i32 x2,
    i32 y2,
    u8 r,
    u8 g,
    u8 b,
    i32 thick) {
    i32 dx = x1 - x2;
    if (dx < 0) {
        dx = -dx;
    }
    i32 dy = y1 - y2;
    if (dy < 0) {
        dy = -dy;
    }
    i32 sx = (x1 < x2) ? 1 : -1;
    i32 sy = (y1 < y2) ? 1 : -1;
    i32 err = dx - dy;
    i32 x = x1;
    i32 y = y1;
    while (true) {
        for (i32 ty = -thick; ty <= thick; ++ty) {
            for (i32 tx = -thick; tx <= thick; ++tx) {
                if (tx * tx + ty * ty > thick * thick) {
                    continue;
                }
                const i32 px = x + tx;
                const i32 py = y + ty;
                if (px < 0 || py < 0 || static_cast<u32>(px) >= static_cast<u32>(w) || static_cast<u32>(py) >= static_cast<u32>(h)) {
                    continue;
                }
                const u32 p = (static_cast<u32>(py) * static_cast<u32>(w) + static_cast<u32>(px)) * 3u;
                rgb[p + 0] = r;
                rgb[p + 1] = g;
                rgb[p + 2] = b;
            }
        }
        if (x == x2 && y == y2) {
            break;
        }
        const i32 e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x += sx;
        }
        if (e2 < dx) {
            err += dx;
            y += sy;
        }
    }
}

//================================================================================================================================
//=> - P1_Gen_RiverNetworkView -
//================================================================================================================================

bool P1_Gen_RiverNetworkView::save_pri (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    const P1_Gen_RiverNetworkRslt& r,
    const P1_Gen_RiverPtsRslt& pts) {
    if (path == nullptr || terrain == nullptr || r.m_ov == nullptr || w == 0 || h == 0 || !pts.m_que.ok()) {
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
        rgb[i * 3u + 0] = cr;
        rgb[i * 3u + 1] = cg;
        rgb[i * 3u + 2] = cb;
    }
    u32 max_basin = 0;
    for (u32 i = 0; i < n; ++i) {
        const u32 bid = static_cast<u32>(r.m_ov[i]);
        if (bid > max_basin) {
            max_basin = bid;
        }
    }
    const u16 pal_cap = static_cast<u16>(max_basin + 1u);
    u8* pal = new u8[static_cast<size_t>(pal_cap) * 3u];
    bool* pal_set = new bool[pal_cap];
    if (pal == nullptr || pal_set == nullptr) {
        delete[] pal_set;
        delete[] pal;
        delete[] rgb;
        return false;
    }
    for (u16 i = 0; i < pal_cap; ++i) {
        pal_set[i] = false;
    }
    Rng32 rng;
    rng_seed(&rng, seed ^ 0xA5A5A5A5u);
    for (u32 i = 0; i < n; ++i) {
        const u16 bid = r.m_ov[i];
        if (bid == static_cast<u16>(P1_RIVER_BASIN_NONE) || bid >= pal_cap) {
            continue;
        }
        if (!pal_set[bid]) {
            pal[bid * 3u + 0] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 1] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal[bid * 3u + 2] = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            pal_set[bid] = true;
        }
        const u32 p = i * 3u;
        rgb[p + 0] = pal[bid * 3u + 0];
        rgb[p + 1] = pal[bid * 3u + 1];
        rgb[p + 2] = pal[bid * 3u + 2];
    }
    if (r.m_downstream != nullptr) {
        const u8 lr = 0;
        const u8 lg = 100;
        const u8 lb = 255;
        for (u32 si = 0; si < static_cast<u32>(r.m_sector_n); ++si) {
            const u16 dn = r.m_downstream[si];
            if (dn == static_cast<u16>(P1_RIVER_DOWN_UNDEF) || dn == static_cast<u16>(P1_RIVER_DOWN_MOUTH)) {
                continue;
            }
            draw_thick_line(
                rgb,
                w,
                h,
                static_cast<i32>(pts.m_que.x_at(si)),
                static_cast<i32>(pts.m_que.y_at(si)),
                static_cast<i32>(pts.m_que.x_at(static_cast<u32>(dn))),
                static_cast<i32>(pts.m_que.y_at(static_cast<u32>(dn))),
                lr,
                lg,
                lb,
                1);
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] pal_set;
    delete[] pal;
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
