//================================================================================================================================
//=> - Include guards -
//================================================================================================================================

#ifndef INTF_MK1_DUMP_UTIL_H
#define INTF_MK1_DUMP_UTIL_H

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "game_map_defs.h"
#include "intf_snapshot_view.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_UI_OUT = "/home/w/Projects/simple-map-gen/game-ui";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static inline bool mk1_file_ok (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static inline bool mk1_ensure_dir (cstr path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static inline void mk1_terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    static const u8* const rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE
    };
    for (unsigned i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        if (rows[i][0] == cls) {
            *r = rows[i][1];
            *g = rows[i][2];
            *b = rows[i][3];
            return;
        }
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static inline bool mk1_wr_rgb_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static inline bool mk1_dump_layers (const Intf_SnapshotView& view, cstr prefix) {
    if (!view.ready() || prefix == nullptr) {
        return false;
    }
    if (!mk1_ensure_dir(G_UI_OUT)) {
        std::printf("FAIL: ensure out dir %s\n", G_UI_OUT);
        return false;
    }
    const u16 w = view.width();
    const u16 h = view.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    char path[512];
    bool ok = true;

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            mk1_terr_rgb(view.get_terrain(x, y), &r, &g, &b);
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    std::snprintf(path, sizeof(path), "%s/%s_01_terrain.ppm", G_UI_OUT, prefix);
    ok = mk1_wr_rgb_ppm(path, rgb, w, h) && ok;
    std::printf("saved: %s\n", path);

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(view.get_climate(x, y), &r, &g, &b);
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    std::snprintf(path, sizeof(path), "%s/%s_02_climate.ppm", G_UI_OUT, prefix);
    ok = mk1_wr_rgb_ppm(path, rgb, w, h) && ok;
    std::printf("saved: %s\n", path);

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            const u8 v = view.get_river(x, y) != 0 ? 255 : 0;
            rgb[i * 3u + 0] = v;
            rgb[i * 3u + 1] = v;
            rgb[i * 3u + 2] = v;
        }
    }
    std::snprintf(path, sizeof(path), "%s/%s_03_rivers.ppm", G_UI_OUT, prefix);
    ok = mk1_wr_rgb_ppm(path, rgb, w, h) && ok;
    std::printf("saved: %s\n", path);

    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            if (overlay_is_water_terr(view.get_terrain(x, y))) {
                mk1_terr_rgb(TERR_OCEAN[0], &r, &g, &b);
            } else {
                u8 clim = view.get_climate(x, y);
                if (clim == CLIMATE_BLACK_SOIL) {
                    clim = CLIMATE_GRASSLAND;
                }
                climate_to_rgb(clim, &r, &g, &b);
            }
            rgb[i * 3u + 0] = r;
            rgb[i * 3u + 1] = g;
            rgb[i * 3u + 2] = b;
        }
    }
    std::snprintf(path, sizeof(path), "%s/%s_04_viz.ppm", G_UI_OUT, prefix);
    ok = mk1_wr_rgb_ppm(path, rgb, w, h) && ok;
    std::printf("saved: %s\n", path);

    delete[] rgb;
    return ok;
}

#endif // INTF_MK1_DUMP_UTIL_H

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
