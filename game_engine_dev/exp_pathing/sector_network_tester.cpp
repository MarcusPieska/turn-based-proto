//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "sector_network.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_IN_TERR = "/home/w/Projects/simple-map-gen/p1-seed-42/terrain.ppm";
static const char* G_IN_CLIM = "/home/w/Projects/simple-map-gen/p1-seed-42/climate.ppm";
static const char* G_IN_RIV = "/home/w/Projects/simple-map-gen/p1-seed-42/rivers.ppm";
static const char* G_IN_OV = "/home/w/Projects/simple-map-gen/p1-seed-42/overlay.ppm";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/sector-network-test";
static const char* G_OUT_PPM = "/home/w/Projects/simple-map-gen/sector-network-test/02_connectivity.ppm";

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool ensure_dir (const char* path) {
    if (path == nullptr || path[0] == '\0') {
        return false;
    }
    return mkdir(path, 0755) == 0 || errno == EEXIST;
}

static bool wr_ppm (const char* path, const u8* rgb, u16 w, u16 h) {
    if (path == nullptr || rgb == nullptr || w == 0 || h == 0) {
        return false;
    }
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

static void put_px (u8* rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    rgb[i * 3u + 0] = r;
    rgb[i * 3u + 1] = g;
    rgb[i * 3u + 2] = b;
}

static void draw_line (u8* rgb, u16 w, u16 h, u16 x0, u16 y0, u16 x1, u16 y1, u8 r, u8 g, u8 b) {
    i32 ax = static_cast<i32>(x0);
    i32 ay = static_cast<i32>(y0);
    const i32 bx = static_cast<i32>(x1);
    const i32 by = static_cast<i32>(y1);
    const i32 dx = (bx > ax) ? (bx - ax) : (ax - bx);
    const i32 dy = (by > ay) ? (by - ay) : (ay - by);
    const i32 sx = (ax < bx) ? 1 : -1;
    const i32 sy = (ay < by) ? 1 : -1;
    i32 err = dx - dy;
    for (;;) {
        put_px(rgb, w, h, ax, ay, r, g, b);
        if (ax == bx && ay == by) {
            break;
        }
        const i32 e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            ax += sx;
        }
        if (e2 < dx) {
            err += dx;
            ay += sy;
        }
    }
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
    } else if (cls == TERR_VOLCANO[0]) {
        *r = TERR_VOLCANO[1]; *g = TERR_VOLCANO[2]; *b = TERR_VOLCANO[3];
    } else if (cls == TERR_INLAND_SEA[0]) {
        *r = TERR_INLAND_SEA[1]; *g = TERR_INLAND_SEA[2]; *b = TERR_INLAND_SEA[3];
    } else if (cls == TERR_INLAND_LAKE[0]) {
        *r = TERR_INLAND_LAKE[1]; *g = TERR_INLAND_LAKE[2]; *b = TERR_INLAND_LAKE[3];
    }
}

static void fill_terr (const GameArraySimple& map, u8* out) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            out[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)] = map.get_terrain(x, y);
        }
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!ensure_dir(G_OUT_DIR)) {
        std::printf("failed to ensure output dir\n");
        return -1;
    }
    GameArraySimple map;
    if (!Factory_GameArraySimple::load_map_gen_data(&map, G_IN_TERR, G_IN_CLIM, G_IN_RIV, G_IN_OV)) {
        std::printf("failed to load map gen data\n");
        return -1;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    std::printf("loaded %u x %u\n", static_cast<u32>(w), static_cast<u32>(h));
    u8* terr = new u8[map.tile_n()];
    fill_terr(map, terr);
    SectorNetwork net;
    if (!net.begin(w, h, terr)) {
        std::printf("failed to begin sector network\n");
        delete[] terr;
        return -1;
    }
    u32 land_n = 0;
    u32 water_n = 0;
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* s = net.get(i);
        for (u32 b = 0; b < 6u; ++b) {
            if ((s->m_mask & static_cast<u8>(1u << b)) != 0) {
                ++land_n;
            }
            if ((s->m_wmask & static_cast<u8>(1u << b)) != 0) {
                ++water_n;
            }
        }
    }
    std::printf("sectors=%u land_links=%u water_links=%u\n",
        static_cast<u32>(net.sector_n()), land_n, water_n);
    u8* rgb = new u8[static_cast<size_t>(map.tile_n()) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 cls = map.get_terrain(x, y);
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            if (overlay_is_water_terr(cls)) {
                r = 0;
                g = 90;
                b = 255;
            } else {
                terr_rgb(cls, &r, &g, &b);
            }
            put_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* a = net.get(i);
        for (u32 bit = 0; bit < 6u; ++bit) {
            const u16 j = net.wnbr(i, static_cast<u8>(bit));
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const Sector* b = net.get(j);
            draw_line(rgb, w, h, a->m_x, a->m_y, b->m_x, b->m_y, 255, 255, 255);
        }
    }
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* a = net.get(i);
        for (u32 bit = 0; bit < 6u; ++bit) {
            const u16 j = net.nbr(i, static_cast<u8>(bit));
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const Sector* b = net.get(j);
            draw_line(rgb, w, h, a->m_x, a->m_y, b->m_x, b->m_y, 0, 0, 0);
        }
    }
    for (u16 i = 0; i < net.sector_n(); ++i) {
        const Sector* s = net.get(i);
        put_px(rgb, w, h, s->m_x, s->m_y, 255, 0, 0);
    }
    if (!wr_ppm(G_OUT_PPM, rgb, w, h)) {
        std::printf("failed to save: %s\n", G_OUT_PPM);
        delete[] rgb;
        delete[] terr;
        return -1;
    }
    delete[] rgb;
    delete[] terr;
    std::printf("saved: %s\n", G_OUT_PPM);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
