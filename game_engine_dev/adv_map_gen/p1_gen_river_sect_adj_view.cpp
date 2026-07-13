//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_sect_adj_view.h"

#include <cstdio>

#include "generator_constants.h"
#include "p1_gen_river_sectors.h"

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

static bool is_wat (u8 cls) {
    return cls == TERR_OCEAN[0] || cls == TERR_SEA[0] || cls == TERR_COASTAL[0];
}

static void base_rgb (u8 cls, u8* r, u8* g, u8* b) {
    if (is_wat(cls)) {
        *r = 48;
        *g = 96;
        *b = 200;
    } else {
        *r = 48;
        *g = 140;
        *b = 48;
    }
}

static bool color_used (const u8* clr, u32 sector_n, u8 r, u8 g, u8 b) {
    for (u32 si = 0; si < sector_n; ++si) {
        if (clr[si * 3u + 0] == r && clr[si * 3u + 1] == g && clr[si * 3u + 2] == b) {
            return true;
        }
    }
    return false;
}

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || static_cast<u32>(x) >= static_cast<u32>(w) || static_cast<u32>(y) >= static_cast<u32>(h)) {
        return;
    }
    const u32 p = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[p + 0] = r;
    rgb[p + 1] = g;
    rgb[p + 2] = b;
}

static void draw_line_1px (u8* rgb, u16 w, u16 h, i32 x1, i32 y1, i32 x2, i32 y2, u8 r, u8 g, u8 b) {
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
        put_px(rgb, w, h, x, y, r, g, b);
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

static bool sec_cxy (const P1_Gen_RiverPtsRslt& pts, u16 si, u16* ox, u16* oy) {
    if (!pts.m_que.ok() || si >= pts.m_n || ox == nullptr || oy == nullptr) {
        return false;
    }
    *ox = pts.m_que.x_at(static_cast<u32>(si));
    *oy = pts.m_que.y_at(static_cast<u32>(si));
    return true;
}

//================================================================================================================================
//=> - P1_Gen_RiverSectAdjView -
//================================================================================================================================

bool P1_Gen_RiverSectAdjView::save_pri (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    const u16* sector_ov,
    u16 sector_n,
    const P1_Gen_RiverSectAdjRslt& adj,
    const P1_Gen_RiverPtsRslt& pts) 
{
    if (path == nullptr || terrain == nullptr || sector_ov == nullptr || w == 0 || h == 0
        || adj.m_nb == nullptr || adj.m_nb_n == nullptr || !pts.m_que.ok()) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u32 sn = static_cast<u32>(sector_n);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        u8 r = 0;
        u8 g = 0;
        u8 b = 0;
        base_rgb(terrain[i], &r, &g, &b);
        rgb[i * 3u + 0] = r;
        rgb[i * 3u + 1] = g;
        rgb[i * 3u + 2] = b;
    }
    u8* clr = new u8[sn * 3u];
    if (clr == nullptr) {
        delete[] rgb;
        return false;
    }
    Rng32 rng;
    rng_seed(&rng, seed);
    for (u32 si = 0; si < sn; ++si) {
        u8 cr = 0;
        u8 cg = 0;
        u8 cb = 0;
        for (i32 tries = 0; tries < 256; ++tries) {
            cr = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            cg = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            cb = static_cast<u8>(50u + (rng_next(&rng) % 151u));
            if (!color_used(clr, si, cr, cg, cb)) {
                break;
            }
        }
        clr[si * 3u + 0] = cr;
        clr[si * 3u + 1] = cg;
        clr[si * 3u + 2] = cb;
    }
    for (u32 i = 0; i < n; ++i) {
        const u16 sid = sector_ov[i];
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE) || sid >= sector_n) {
            continue;
        }
        const u32 p = i * 3u;
        const u32 c = static_cast<u32>(sid) * 3u;
        rgb[p + 0] = clr[c + 0];
        rgb[p + 1] = clr[c + 1];
        rgb[p + 2] = clr[c + 2];
    }
    const u8 lr = 24;
    const u8 lg = 24;
    const u8 lb = 24;
    for (u16 si = 0; si < sector_n; ++si) {
        u16 ax = 0;
        u16 ay = 0;
        if (!sec_cxy(pts, si, &ax, &ay)) {
            continue;
        }
        const u8 nn = adj.m_nb_n[si];
        for (u8 k = 0; k < nn; ++k) {
            const u16 nb = p1_sect_adj_nb(adj, si, k);
            if (nb == static_cast<u16>(P1_RIVER_SECTOR_NONE) || nb >= sector_n || nb <= si) {
                continue;
            }
            u16 bx = 0;
            u16 by = 0;
            if (!sec_cxy(pts, nb, &bx, &by)) {
                continue;
            }
            draw_line_1px(rgb, w, h, static_cast<i32>(ax), static_cast<i32>(ay), static_cast<i32>(bx), static_cast<i32>(by), lr, lg, lb);
        }
    }
    delete[] clr;
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
