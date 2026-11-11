//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_xfer_save_snap.h"

#include <cstdio>

#include "build_adds_array.h"
#include "draw_map_tile_ownership.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static const u8 k_road_gray = 64u;
static const u8 k_mtn_a = 230u; // Mountain overlay opacity; leftover is underdraw
static const u8 k_mtn_r = 42u;
static const u8 k_mtn_g = 24u;
static const u8 k_mtn_b = 12u;

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

//================================================================================================================================
//=> - MapOwnXferSaveSnap -
//================================================================================================================================

static bool paint_base (u8* rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    if (rgb == nullptr || w == 0 || h == 0) {
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
            bleach(&r, &g, &b);
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    return true;
}

static void paint_roads_cities (u8* rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
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
}

static bool mtn_rim (const GameArraySimple& map, u16 x, u16 y) {
    static const i32 dx[4] = {1, -1, 0, 0};
    static const i32 dy[4] = {0, 0, 1, -1};
    const u16 w = map.width();
    const u16 h = map.height();
    for (u8 k = 0; k < 4u; ++k) {
        const i32 nx = static_cast<i32>(x) + dx[k];
        const i32 ny = static_cast<i32>(y) + dy[k];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
            return true;
        }
        if (map.get_terrain(static_cast<u16>(nx), static_cast<u16>(ny)) != TERR_MOUNTAINS[0]) {
            return true;
        }
    }
    return false;
}

static void paint_mountains (u8* rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u8 a = k_mtn_a;
    const u8 ia = static_cast<u8>(255u - a);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_terrain(x, y) != TERR_MOUNTAINS[0]) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
            rgb[i + 0] = static_cast<u8>((static_cast<u16>(k_mtn_r) * a + static_cast<u16>(rgb[i + 0]) * ia) / 255u);
            rgb[i + 1] = static_cast<u8>((static_cast<u16>(k_mtn_g) * a + static_cast<u16>(rgb[i + 1]) * ia) / 255u);
            rgb[i + 2] = static_cast<u8>((static_cast<u16>(k_mtn_b) * a + static_cast<u16>(rgb[i + 2]) * ia) / 255u);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_terrain(x, y) != TERR_MOUNTAINS[0] || !mtn_rim(map, x, y)) {
                continue;
            }
            set_px(rgb, w, h, x, y, 0, 0, 0);
        }
    }
}

static bool wr_ppm (cstr path, u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

bool MapOwnXferSaveSnap::write_seats (cstr path, const GameArraySimple& base, const u8* seats) {
    if (path == nullptr || seats == nullptr) {
        return false;
    }
    const u16 w = base.width();
    const u16 h = base.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    if (!paint_base(rgb, base)) {
        delete[] rgb;
        return false;
    }
    u8* own = new u8[static_cast<size_t>(n)];
    if (own == nullptr) {
        delete[] rgb;
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (seats[i] == U8_KEY_NULL || is_open_water(base.get_terrain(x, y))) {
                own[i] = U8_KEY_NULL;
            } else {
                own[i] = seats[i];
            }
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (own[i] == U8_KEY_NULL) {
                continue;
            }
            DrawMapTileOwnership::blend(rgb, w, h, x, y, own);
        }
    }
    delete[] own;
    paint_roads_cities(rgb, base);
    paint_mountains(rgb, base);
    const bool ok = wr_ppm(path, rgb, w, h);
    delete[] rgb;
    return ok;
}

bool MapOwnXferSaveSnap::write (cstr path, const GameArraySimple& cur, const GameArraySimple& prev) {
    if (path == nullptr) {
        return false;
    }
    const u16 w = cur.width();
    const u16 h = cur.height();
    if (w == 0 || h == 0 || prev.width() != w || prev.height() != h) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* seats = new u8[n];
    if (seats == nullptr) {
        return false;
    }
    for (u32 i = 0; i < n; ++i) {
        seats[i] = U8_KEY_NULL;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 now = cur.get_civ_owner(x, y);
            const u8 was = prev.get_civ_owner(x, y);
            if (now == was || was == U8_KEY_NULL || now == U8_KEY_NULL) {
                continue;
            }
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            seats[i] = now;
        }
    }
    const bool ok = write_seats(path, cur, seats);
    delete[] seats;
    return ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
