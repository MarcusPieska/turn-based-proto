//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "map_own_xfer_save_snap.h"

#include <cstdio>

#include "build_adds_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Palette -
//================================================================================================================================

static const u8 k_own_pal[][3] = {
    {220, 40, 40},
    {40, 90, 220},
    {40, 170, 70},
    {220, 110, 30},
    {190, 40, 170},
    {30, 170, 170},
    {150, 70, 30},
    {100, 40, 180},
    {200, 160, 40},
    {40, 140, 200},
    {160, 50, 90},
    {90, 140, 40},
    {137, 25, 25},
    {25, 56, 137},
    {25, 106, 44},
    {137, 69, 19},
    {119, 25, 106},
    {19, 106, 106},
    {94, 44, 19},
    {62, 25, 112},
    {125, 100, 25},
    {25, 87, 125},
    {100, 31, 56},
    {56, 87, 25},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));
static const u8 k_road_gray = 64u;

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

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

static void shade_own (u8* rgb, u16 w, u16 h, u16 x, u16 y, u16 seat) {
    if (x >= w || y >= h) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    const u16 cr = (static_cast<u16>(c[0]) * 5u) / 8u;
    const u16 cg = (static_cast<u16>(c[1]) * 5u) / 8u;
    const u16 cb = (static_cast<u16>(c[2]) * 5u) / 8u;
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) + cr * 3u) / 4u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) + cg * 3u) / 4u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) + cb * 3u) / 4u);
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
            if (terr == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
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
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u8 seat = seats[i];
            if (seat == U8_KEY_NULL) {
                continue;
            }
            if (is_open_water(base.get_terrain(x, y))) {
                continue;
            }
            shade_own(rgb, w, h, x, y, static_cast<u16>(seat));
        }
    }
    paint_roads_cities(rgb, base);
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
