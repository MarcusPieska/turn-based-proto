//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_sect_order_snap.h"

#include <cstdio>

#include "build_adds_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u8 k_road_gray = 64u;
static const u8 k_rim_gray = 128u;

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
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
    } else if (cls == TERR_INLAND_SEA[0]) {
        *r = TERR_INLAND_SEA[1]; *g = TERR_INLAND_SEA[2]; *b = TERR_INLAND_SEA[3];
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = TERR_INLAND_LAKE[1]; *g = TERR_INLAND_LAKE[2]; *b = TERR_INLAND_LAKE[3];
    } else if (cls == TERR_PLAINS[0]) {
        *r = TERR_PLAINS[1]; *g = TERR_PLAINS[2]; *b = TERR_PLAINS[3];
    } else if (cls == TERR_HILLS[0]) {
        *r = TERR_HILLS[1]; *g = TERR_HILLS[2]; *b = TERR_HILLS[3];
    } else if (cls == TERR_MOUNTAINS[0]) {
        *r = TERR_MOUNTAINS[1]; *g = TERR_MOUNTAINS[2]; *b = TERR_MOUNTAINS[3];
    }
}

static bool is_open_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
}

static bool is_water_base (u8 terr) {
    return is_open_water(terr) || terr == TERR_INLAND_SEA[0] || terr == TERR_INLAND_LAKE[0];
}

static void bleach (u8* r, u8* g, u8* b) {
    const u16 gray = (static_cast<u16>(*r) + static_cast<u16>(*g) + static_cast<u16>(*b)) / 3u;
    *r = static_cast<u8>((gray + 255u * 2u) / 3u);
    *g = static_cast<u8>((gray + 255u * 2u) / 3u);
    *b = static_cast<u8>((gray + 255u * 2u) / 3u);
}

static void order_rgb (u16 step, u16 n, u8* r, u8* g, u8* b) {
    float t = 0.0f;
    if (n > 1u) {
        t = static_cast<float>(step) / static_cast<float>(n - 1u);
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    *r = 20u;
    *g = static_cast<u8>((1.0f - t) * 220.0f + 20.0f);
    *b = static_cast<u8>(t * 220.0f + 20.0f);
}

static bool is_rim (const u16* paint, u16 w, u16 h, u16 x, u16 y, u16 tag) {
    static const i32 dx[4] = {1, -1, 0, 0};
    static const i32 dy[4] = {0, 0, 1, -1};
    for (u8 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + dx[k];
        const i32 ny = static_cast<i32>(y) + dy[k];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
            return true;
        }
        const u32 ni = static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx);
        if (paint[ni] != tag) {
            return true;
        }
    }
    return false;
}

//================================================================================================================================
//=> - LuckySectOrderSnap -
//================================================================================================================================

bool LuckySectOrderSnap::write (
    cstr path,
    const GameArraySimple& map,
    const u16* paint,
    u16 pw,
    u16 ph,
    u16 sec_n,
    const u16* order,
    u16 order_n,
    const u8* skipped) {
    if (path == nullptr || paint == nullptr || sec_n == 0u) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0u || h == 0u || w != pw || h != ph) {
        return false;
    }
    if (order_n > 0u && order == nullptr) {
        return false;
    }
    u16* rank = new u16[sec_n];
    if (rank == nullptr) {
        return false;
    }
    for (u16 i = 0; i < sec_n; ++i) {
        rank[i] = U16_KEY_NULL;
    }
    u16 used_n = 0;
    for (u16 i = 0; i < order_n; ++i) {
        const u16 s = order[i];
        if (s >= sec_n || rank[s] != U16_KEY_NULL) {
            continue;
        }
        rank[s] = used_n;
        used_n = static_cast<u16>(used_n + 1u);
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        delete[] rank;
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            const u8 terr = map.get_terrain(x, y);
            if (is_water_base(terr)) {
                terr_rgb(terr, &r, &g, &b);
            } else {
                climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            }
            if (map.get_river(x, y) != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            if (terr == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            bleach(&r, &g, &b);
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u16 tag = paint[i];
            if (tag != 0u && tag <= sec_n) {
                const u16 id = static_cast<u16>(tag - 1u);
                if (skipped != nullptr && skipped[id] != 0u) {
                    r = 220u;
                    g = 40u;
                    b = 40u;
                } else if (rank[id] != U16_KEY_NULL) {
                    order_rgb(rank[id], used_n > 0u ? used_n : 1u, &r, &g, &b);
                }
            }
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u16 tag = paint[i];
            if (tag == 0u || tag > sec_n) {
                continue;
            }
            if (!is_rim(paint, w, h, x, y, tag)) {
                continue;
            }
            const u16 id = static_cast<u16>(tag - 1u);
            if (rank[id] != U16_KEY_NULL || (skipped != nullptr && skipped[id] != 0u)) {
                set_px(rgb, w, h, x, y, 0u, 0u, 0u);
            } else {
                set_px(rgb, w, h, x, y, k_rim_gray, k_rim_gray, k_rim_gray);
            }
        }
    }
    delete[] rank;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (!road_is_built(map.get_road_typ(x, y))) {
                continue;
            }
            set_px(rgb, w, h, x, y, k_road_gray, k_road_gray, k_road_gray);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_add_typ(x, y) != static_cast<u8>(BUILD_ADD_CITY)) {
                continue;
            }
            set_px(rgb, w, h, x, y, 0, 0, 0);
        }
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
