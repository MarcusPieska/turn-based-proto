//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "p1_gen_ocean_index_view.h"

#include <cstdio>

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

static bool color_used (const u8* pal, u16 pal_n, u8 r, u8 g, u8 b) {
    for (u16 i = 0; i < pal_n; ++i) {
        if (pal[static_cast<u32>(i) * 3u + 0] == r && pal[static_cast<u32>(i) * 3u + 1] == g && pal[static_cast<u32>(i) * 3u + 2] == b) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - P1_Gen_OceanIndexView -
//================================================================================================================================

bool P1_Gen_OceanIndexView::save_pri (
    cstr path,
    u16 w,
    u16 h,
    u32 seed,
    const P1_Gen_OceanIndexRslt& r) {
    if (path == nullptr || w == 0 || h == 0 || r.m_ov.data() == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const u16* ov = r.m_ov.data();
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        rgb[i * 3u + 0] = 0;
        rgb[i * 3u + 1] = 0;
        rgb[i * 3u + 2] = 0;
    }
    const u16 pal_n = r.m_ocean_n;
    u8* pal = nullptr;
    if (pal_n > 0u) {
        pal = new u8[static_cast<size_t>(pal_n) * 3u];
        if (pal == nullptr) {
            delete[] rgb;
            return false;
        }
        Rng32 rng;
        rng_seed(&rng, seed ^ 0x0CE4A1u);
        for (u16 oi = 0; oi < pal_n; ++oi) {
            u8 cr = 0;
            u8 cg = 0;
            u8 cb = 0;
            for (u32 guard = 0; guard < 256u; ++guard) {
                cr = static_cast<u8>(160u + (rng_next(&rng) % 96u));
                cg = static_cast<u8>(160u + (rng_next(&rng) % 96u));
                cb = static_cast<u8>(160u + (rng_next(&rng) % 96u));
                if (!color_used(pal, oi, cr, cg, cb)) {
                    break;
                }
            }
            pal[static_cast<u32>(oi) * 3u + 0] = cr;
            pal[static_cast<u32>(oi) * 3u + 1] = cg;
            pal[static_cast<u32>(oi) * 3u + 2] = cb;
        }
        for (u32 i = 0; i < n; ++i) {
            const u16 idx = ov[i];
            if (idx == static_cast<u16>(P1_OCEAN_IDX_NONE) || idx > pal_n) {
                continue;
            }
            const u32 pi = (static_cast<u32>(idx) - 1u) * 3u;
            const u32 p = i * 3u;
            rgb[p + 0] = pal[pi + 0];
            rgb[p + 1] = pal[pi + 1];
            rgb[p + 2] = pal[pi + 2];
        }
    }
    const bool ok = save_rgb_ppm(path, rgb, w, h);
    delete[] pal;
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
