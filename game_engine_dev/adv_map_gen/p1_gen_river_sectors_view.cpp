//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_river_sectors_view.h"

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

static bool color_used (const u8* clr, u32 sector_n, u8 r, u8 g, u8 b) {
    for (u32 si = 0; si < sector_n; ++si) {
        if (clr[si * 3u + 0] == r && clr[si * 3u + 1] == g && clr[si * 3u + 2] == b) {
            return true;
        }
    }
    return false;
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
//=> - P1_Gen_RiverSectorsView -
//================================================================================================================================

bool P1_Gen_RiverSectorsView::save_pri (
    cstr path,
    const u8* terrain,
    u16 w,
    u16 h,
    u32 seed,
    const u16* sector_ov,
    u16 sector_n) 
{
    if (path == nullptr || terrain == nullptr || sector_ov == nullptr || w == 0 || h == 0) {
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
        terr_rgb(terrain[i], &r, &g, &b);
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
        if (sid == static_cast<u16>(P1_RIVER_SECTOR_NONE)) {
            continue;
        }
        const u32 p = i * 3u;
        const u32 c = static_cast<u32>(sid) * 3u;
        rgb[p + 0] = clr[c + 0];
        rgb[p + 1] = clr[c + 1];
        rgb[p + 2] = clr[c + 2];
    }
    delete[] clr;
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
