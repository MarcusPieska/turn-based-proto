//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_walkable_sectors.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "sector_network.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_dir[256];

int test_count = 0;
int test_pass = 0;
int total_test_fails = 0;
int total_tests_run = 0;
int print_level = 1;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

void note_result (bool cond, cstr msg) {
    test_count++;
    total_tests_run++;
    if (cond) {
        test_pass++;
        if (print_level > 1) {
            std::printf("*** TEST PASSED: %s\n", msg);
        }
    } else {
        total_test_fails++;
        if (print_level > 0) {
            std::printf("*** TEST FAILED: %s\n", msg);
        }
    }
}

static bool build_paths () {
    if (std::snprintf(g_dir, sizeof(g_dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", g_dir) <= 0) {
        return false;
    }
    return true;
}

static void sec_rgb (u16 id, u8* r, u8* g, u8* b) {
    *r = static_cast<u8>(40u + ((static_cast<u32>(id) * 37u) % 200u));
    *g = static_cast<u8>(40u + ((static_cast<u32>(id) * 61u) % 200u));
    *b = static_cast<u8>(40u + ((static_cast<u32>(id) * 91u) % 200u));
}

static void terr_rgb (u8 cls, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS,
        TERR_MOUNTAINS, TERR_VOLCANO, TERR_INLAND_SEA, TERR_INLAND_LAKE};
    for (unsigned i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == cls) {
            *r = k_rows[i][1];
            *g = k_rows[i][2];
            *b = k_rows[i][3];
            return;
        }
    }
    *r = 0;
    *g = 0;
    *b = 0;
}

static bool write_ppm (cstr path, const std::vector<u8>& rgb, u16 w, u16 h) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

static bool write_sec_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenWalkableSectors& gws,
    const SectorNetwork& net)
{
    const Whiteboard_2B& sec = gws.sectors();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            const u16 tag = sec.rd(x, y);
            if (tag != GWS_IDX_NONE) {
                sec_rgb(static_cast<u16>(tag - 1u), &r, &g, &b);
            } else {
                terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    for (u16 id = 0; id < net.sector_n(); ++id) {
        const Sector* s = net.get(id);
        if (s == nullptr) {
            continue;
        }
        const u32 i = (static_cast<u32>(s->m_y) * w + static_cast<u32>(s->m_x)) * 3u;
        rgb[i] = 255u;
        rgb[i + 1] = 0u;
        rgb[i + 2] = 0u;
    }
    return write_ppm(path, rgb, w, h);
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

static double ms_since (std::chrono::steady_clock::time_point t0) {
    const auto dt = std::chrono::steady_clock::now() - t0;
    return std::chrono::duration<double, std::milli>(dt).count();
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    note_result(build_paths(), "build map paths");

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    note_result(terr.width() > 0 && terr.height() > 0, "terrain size");
    const u16 mw = terr.width();
    const u16 mh = terr.height();

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(map.width() == mw && map.height() == mh, "map size matches");

    u8* terr_u8 = new u8[map.tile_n()];
    fill_terr(map, terr_u8);
    SectorNetwork net;
    note_result(net.begin(mw, mh, terr_u8), "SectorNetwork::begin");
    delete[] terr_u8;

    WhiteboardMng::init(mw, mh);
    note_result(WhiteboardMng::width() == mw && WhiteboardMng::height() == mh, "WhiteboardMng::init");

    GenWalkableSectors gws;
    note_result(gws.begin(net, map), "GenWalkableSectors::begin");

    auto t0 = std::chrono::steady_clock::now();
    const bool build_ok = gws.build();
    const double build_ms = ms_since(t0);
    note_result(build_ok, "GenWalkableSectors::build");
    note_result(gws.sector_n() > 0u, "sector_n > 0");
    note_result(gws.paint_n() > 0u, "paint_n > 0");
    std::printf("build: sectors=%u paint_tiles=%u  wall_ms=%.3f\n",
        gws.sector_n(), gws.paint_n(), build_ms);

    char ppm_path[384];
    std::snprintf(ppm_path, sizeof(ppm_path), "%s/gen_walkable_sectors.ppm", g_dir);
    note_result(write_sec_ppm(ppm_path, map, gws, net), ppm_path);
    std::printf("wrote %s\n", ppm_path);

    WhiteboardMng::terminate();

    std::printf("=======================================================\n");
    std::printf(" GEN WALKABLE SECTORS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
