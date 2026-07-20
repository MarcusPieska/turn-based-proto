//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "hlp_make_tile_owner_map.h"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

#include "game_array_simple.h"
#include "game_map_defs.h"
#include "hlp_parse_seed.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_OUT_DIR = "/home/w/Projects/game-state";

static const u32 k_tiles_magic = 0x534c4954u;
static const u32 k_cities_magic = 0x53495443u;
static const u32 k_io_ver_min = 1u;

//================================================================================================================================
//=> - Colors -
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
    {240, 200, 40},
    {40, 200, 220},
    {180, 80, 80},
    {80, 80, 200},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));

static const u8* k_terr_rgb[] = {
    TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
    TERR_INLAND_SEA, TERR_INLAND_LAKE
};
static const u16 k_terr_rgb_n = static_cast<u16>(sizeof(k_terr_rgb) / sizeof(k_terr_rgb[0]));

//================================================================================================================================
//=> - Dump records -
//================================================================================================================================

struct CityDumpRec {
    u16 m_idx;
    u16 m_owner;
    u16 m_x;
    u16 m_y;
    u16 m_pop;
    u16 m_food;
    u16 m_prod;
    u16 m_culture;
};

struct MapDump {
    u16 w;
    u16 h;
    GameTileSimple* tiles;
};

struct CityPts {
    u16 n;
    u16* x;
    u16* y;
};

//================================================================================================================================
//=> - Load -
//================================================================================================================================

static bool load_map (cstr path, MapDump* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    out->w = 0;
    out->h = 0;
    out->tiles = nullptr;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&w, sizeof(w), 1, fp) != 1
        || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_tiles_magic || ver < k_io_ver_min || w == 0 || h == 0) {
        std::fclose(fp);
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    GameTileSimple* tiles = new GameTileSimple[n];
    if (std::fread(tiles, sizeof(GameTileSimple), n, fp) != n) {
        delete[] tiles;
        std::fclose(fp);
        return false;
    }
    std::fclose(fp);
    out->w = w;
    out->h = h;
    out->tiles = tiles;
    return true;
}

static bool load_city_pts (cstr path, CityPts* out) {
    if (path == nullptr || out == nullptr) {
        return false;
    }
    out->n = 0;
    out->x = nullptr;
    out->y = nullptr;
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    u32 magic = 0;
    u32 ver = 0;
    u16 cn = 0;
    if (std::fread(&magic, sizeof(magic), 1, fp) != 1
        || std::fread(&ver, sizeof(ver), 1, fp) != 1
        || std::fread(&cn, sizeof(cn), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (magic != k_cities_magic || ver < k_io_ver_min) {
        std::fclose(fp);
        return false;
    }
    u16* xs = new u16[cn == 0 ? 1u : cn];
    u16* ys = new u16[cn == 0 ? 1u : cn];
    u16 live = 0;
    for (u16 i = 0; i < cn; ++i) {
        CityDumpRec rec = {};
        if (std::fread(&rec, sizeof(rec), 1, fp) != 1) {
            delete[] xs;
            delete[] ys;
            std::fclose(fp);
            return false;
        }
        if (rec.m_owner == U16_KEY_NULL || rec.m_x == U16_KEY_NULL || rec.m_y == U16_KEY_NULL) {
            continue;
        }
        xs[live] = rec.m_x;
        ys[live] = rec.m_y;
        live = static_cast<u16>(live + 1u);
    }
    std::fclose(fp);
    out->n = live;
    out->x = xs;
    out->y = ys;
    return true;
}

//================================================================================================================================
//=> - Draw -
//================================================================================================================================

static void terr_to_rgb (u8 terr, u8* r, u8* g, u8* b) {
    for (u16 i = 0; i < k_terr_rgb_n; ++i) {
        if (k_terr_rgb[i][0] == terr) {
            *r = k_terr_rgb[i][1];
            *g = k_terr_rgb[i][2];
            *b = k_terr_rgb[i][3];
            return;
        }
    }
    *r = 40;
    *g = 40;
    *b = 40;
}

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void blend_own (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 seat) {
    if (x >= w || y >= h) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) * 1u + static_cast<u16>(c[0]) * 7u) / 8u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) * 1u + static_cast<u16>(c[1]) * 7u) / 8u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) * 1u + static_cast<u16>(c[2]) * 7u) / 8u);
}

static void paint_plus3 (u8* rgb, u16 w, u16 h, u16 x, u16 y) {
    set_px(rgb, w, h, x, y, 0, 0, 0);
    if (x > 0) {
        set_px(rgb, w, h, static_cast<u16>(x - 1u), y, 0, 0, 0);
    }
    if (static_cast<u32>(x) + 1u < static_cast<u32>(w)) {
        set_px(rgb, w, h, static_cast<u16>(x + 1u), y, 0, 0, 0);
    }
    if (y > 0) {
        set_px(rgb, w, h, x, static_cast<u16>(y - 1u), 0, 0, 0);
    }
    if (static_cast<u32>(y) + 1u < static_cast<u32>(h)) {
        set_px(rgb, w, h, x, static_cast<u16>(y + 1u), 0, 0, 0);
    }
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool write_ppm (cstr path, const u8* rgb, u16 w, u16 h) {
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

static bool render (const MapDump& map, const CityPts& cities, cstr out_path) {
    const u16 w = map.w;
    const u16 h = map.h;
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const GameTileSimple& t = map.tiles[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_to_rgb(static_cast<u8>(t.m_terr), &r, &g, &b);
            if (t.m_riv != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const GameTileSimple& t = map.tiles[static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)];
            if (t.m_civ_owner != U8_KEY_NULL) {
                blend_own(rgb, w, h, x, y, static_cast<u8>(t.m_civ_owner));
            }
        }
    }
    for (u16 i = 0; i < cities.n; ++i) {
        paint_plus3(rgb, w, h, cities.x[i], cities.y[i]);
    }
    const bool ok = write_ppm(out_path, rgb, w, h);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Helper_MakeTileOwnerMap -
//================================================================================================================================

bool Helper_MakeTileOwnerMap::run (u32 turn) {
    if (!Helper_ParseSeed::load()) {
        std::printf("Helper_MakeTileOwnerMap: parse_seed load failed\n");
        return false;
    }
    char map_path[384];
    char cities_path[384];
    char out_path[384];
    if (!Helper_ParseSeed::map_path(turn, map_path, sizeof(map_path))
        || !Helper_ParseSeed::cities_path(turn, cities_path, sizeof(cities_path))) {
        return false;
    }
    if (std::snprintf(out_path, sizeof(out_path), "%s/tile_ownership_%04u.ppm", G_OUT_DIR, turn) <= 0) {
        return false;
    }
    if (!ensure_out_dir()) {
        std::printf("Helper_MakeTileOwnerMap: out dir failed: %s\n", G_OUT_DIR);
        return false;
    }
    MapDump map = {};
    if (!load_map(map_path, &map)) {
        std::printf("Helper_MakeTileOwnerMap: load map failed: %s\n", map_path);
        return false;
    }
    CityPts cities = {};
    if (!load_city_pts(cities_path, &cities)) {
        std::printf("Helper_MakeTileOwnerMap: load cities failed: %s\n", cities_path);
        delete[] map.tiles;
        return false;
    }
    if (!render(map, cities, out_path)) {
        std::printf("Helper_MakeTileOwnerMap: render failed: %s\n", out_path);
        delete[] cities.x;
        delete[] cities.y;
        delete[] map.tiles;
        return false;
    }
    std::printf("wrote %s (%u x %u, cities=%u)\n", out_path, map.w, map.h, cities.n);
    delete[] cities.x;
    delete[] cities.y;
    delete[] map.tiles;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
