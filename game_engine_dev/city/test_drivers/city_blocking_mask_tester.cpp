//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "city_blocking_mask.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* k_root = "/home/w/Projects/simple-map-gen/p1-seed-42";
static const char* k_out = "/home/w/Projects/simple-map-gen/city_blocking_mask.ppm";
static const u32 k_seed = 42u;
static const u8 k_blend = 140u;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool is_land (u8 cls) {
    return !overlay_is_water_terr(cls) && cls != TERR_MOUNTAINS[0] && cls != TERR_NONE[0];
}

static bool pick_land (const GameArraySimple& map, u32 seed, u16* ox, u16* oy) {
    const u16 w = map.width();
    const u16 h = map.height();
    if (w == 0 || h == 0 || ox == nullptr || oy == nullptr) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u32 s = seed * 1664525u + 1013904223u;
    for (u32 tries = 0; tries < n; ++tries) {
        s = s * 1664525u + 1013904223u;
        const u32 i = s % n;
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        if (!is_land(map.get_terrain(x, y))) {
            continue;
        }
        *ox = x;
        *oy = y;
        return true;
    }
    return false;
}

static void blend_red (u8* p) {
    const u16 t = static_cast<u16>(255u - k_blend);
    p[0] = static_cast<u8>((static_cast<u16>(p[0]) * t + 255u * k_blend) / 255u);
    p[1] = static_cast<u8>((static_cast<u16>(p[1]) * t) / 255u);
    p[2] = static_cast<u8>((static_cast<u16>(p[2]) * t) / 255u);
}

static bool save_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    char terr[320];
    char clim[320];
    char riv[320];
    std::snprintf(terr, sizeof(terr), "%s/terrain.ppm", k_root);
    std::snprintf(clim, sizeof(clim), "%s/climate.ppm", k_root);
    std::snprintf(riv, sizeof(riv), "%s/rivers.ppm", k_root);
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, terr, clim, riv, nullptr)) {
        std::printf("fail load map\n");
        return 1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    u16 cx = 0;
    u16 cy = 0;
    if (!pick_land(map, k_seed, &cx, &cy)) {
        std::printf("fail pick land\n");
        return 1;
    }
    CityBlockingMask::stamp(map, cx, cy);
    const u8* prev = CityBlockingMask::preview(map, cx, cy);
    u32 stamp_n = 0;
    u32 prev_n = 0;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_settler_blocked(x, y) != 0) {
                stamp_n = stamp_n + 1u;
            }
        }
    }
    for (u32 i = 0; i < CityBlockingMask::m_n; ++i) {
        if (prev[i] != 0) {
            prev_n = prev_n + 1u;
        }
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return 1;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
            rgb[i + 0] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
            if (map.get_river(x, y) != 0) {
                rgb[i + 0] = static_cast<u8>((static_cast<u16>(r) + 40u) / 2u);
                rgb[i + 1] = static_cast<u8>((static_cast<u16>(g) + 120u) / 2u);
                rgb[i + 2] = static_cast<u8>((static_cast<u16>(b) + 220u) / 2u);
            }
            if (map.get_settler_blocked(x, y) != 0) {
                blend_red(&rgb[i]);
            }
        }
    }
    rgb[(static_cast<u32>(cy) * w + static_cast<u32>(cx)) * 3u + 0] = 0;
    rgb[(static_cast<u32>(cy) * w + static_cast<u32>(cx)) * 3u + 1] = 0;
    rgb[(static_cast<u32>(cy) * w + static_cast<u32>(cx)) * 3u + 2] = 0;
    if (!save_ppm(k_out, rgb, w, h)) {
        delete[] rgb;
        std::printf("fail write %s\n", k_out);
        return 1;
    }
    delete[] rgb;
    std::printf("city=(%u,%u) stamp_n=%u prev_n=%u wrote %s\n",
        cx, cy, stamp_n, prev_n, k_out);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
