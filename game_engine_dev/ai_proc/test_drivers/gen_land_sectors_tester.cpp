//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_land_sectors.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "runtime_static_loader.h"
#include "tile_attr_tables.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/gen-land-sectors";
static const u32 G_SEED = 43u;
static const u32 G_RNG = 43u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_flags[320];
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
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", g_dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_flags, sizeof(g_flags), "%s/flags.ppm", g_dir) <= 0) {
        return false;
    }
    return true;
}

static void sec_rgb (u16 id, u8* r, u8* g, u8* b) {
    *r = static_cast<u8>(80u + ((static_cast<u32>(id) * 37u) % 140u));
    *g = static_cast<u8>(80u + ((static_cast<u32>(id) * 61u) % 140u));
    *b = static_cast<u8>(80u + ((static_cast<u32>(id) * 91u) % 140u));
}

static void val_rgb (u32 v, u32 lo, u32 hi, u8* r, u8* g, u8* b) {
    f32 t = 0.5f;
    if (hi > lo) {
        t = static_cast<f32>(v - lo) / static_cast<f32>(hi - lo);
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    *r = static_cast<u8>(255.0f * (1.0f - t) + 0.5f);
    *g = static_cast<u8>(255.0f * t + 0.5f);
    *b = 0u;
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

static void paint_base (const GameArraySimple& map, std::vector<u8>& rgb) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = map.get_terrain(x, y);
            u8 r = 48u;
            u8 g = 48u;
            u8 b = 48u;
            if (overlay_is_water_terr(t)) {
                r = 255u;
                g = 255u;
                b = 255u;
            } else if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
                r = 0u;
                g = 0u;
                b = 0u;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static bool write_points_ppm (cstr path, const GameArraySimple& map, const LandSectorSeeds& seeds) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    paint_base(map, rgb);
    for (u16 i = 0; i < seeds.m_n; ++i) {
        const u16 x = seeds.m_pts[i].m_x;
        const u16 y = seeds.m_pts[i].m_y;
        for (i32 dy = -1; dy <= 1; ++dy) {
            for (i32 dx = -1; dx <= 1; ++dx) {
                const i32 nx = static_cast<i32>(x) + dx;
                const i32 ny = static_cast<i32>(y) + dy;
                if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                    continue;
                }
                const u32 pi = (static_cast<u32>(ny) * w + static_cast<u32>(nx)) * 3u;
                rgb[pi] = 220u;
                rgb[pi + 1] = 40u;
                rgb[pi + 2] = 40u;
            }
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_sectors_ppm (cstr path, const GameArraySimple& map, const GenLandSectors& gls) {
    const Whiteboard_2B& sec = gls.sectors();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = map.get_terrain(x, y);
            u8 r = 32u;
            u8 g = 32u;
            u8 b = 32u;
            if (overlay_is_water_terr(t)) {
                r = 255u;
                g = 255u;
                b = 255u;
            } else if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
                r = 0u;
                g = 0u;
                b = 0u;
            } else {
                const u16 tag = sec.rd(x, y);
                if (tag != GLS_IDX_NONE) {
                    sec_rgb(static_cast<u16>(tag - 1u), &r, &g, &b);
                }
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_value_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const LandSectorSeeds& seeds,
    bool use_yields)
{
    if (seeds.m_pts == nullptr || seeds.m_n == 0u) {
        return false;
    }
    u32 lo = use_yields ? seeds.m_pts[0].m_yields : seeds.m_pts[0].m_res;
    u32 hi = lo;
    for (u16 i = 1; i < seeds.m_n; ++i) {
        const u32 v = use_yields ? seeds.m_pts[i].m_yields : seeds.m_pts[i].m_res;
        if (v < lo) {
            lo = v;
        }
        if (v > hi) {
            hi = v;
        }
    }
    const Whiteboard_2B& sec = gls.sectors();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 t = map.get_terrain(x, y);
            u8 r = 32u;
            u8 g = 32u;
            u8 b = 32u;
            if (overlay_is_water_terr(t)) {
                r = 255u;
                g = 255u;
                b = 255u;
            } else if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
                r = 0u;
                g = 0u;
                b = 0u;
            } else {
                const u16 tag = sec.rd(x, y);
                if (tag != GLS_IDX_NONE) {
                    const u16 si = static_cast<u16>(tag - 1u);
                    if (si < seeds.m_n) {
                        const u32 v = use_yields ? seeds.m_pts[si].m_yields : seeds.m_pts[si].m_res;
                        val_rgb(v, lo, hi, &r, &g, &b);
                    }
                }
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
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
    note_result(::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST, "ensure out dir");

    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "RuntimeStaticLoader::load");
    note_result(TileAttrTables::setup(loader.statics()), "TileAttrTables::setup");

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");
    note_result(Factory_GameArraySimple::load_flags_data(&map, g_flags), "load flags");
    note_result(map.width() > 0 && map.height() > 0, "map size");

    WhiteboardMng::init(map.width(), map.height());
    note_result(WhiteboardMng::width() == map.width(), "WhiteboardMng::init");

    GenLandSectors gls;
    note_result(gls.begin(map), "GenLandSectors::begin");

    LandSectorSeeds seeds = {};
    auto t0 = std::chrono::steady_clock::now();
    const bool seeds_ok = gls.gen_seeds(G_RNG, &seeds);
    const double seeds_ms = ms_since(t0);
    note_result(seeds_ok, "GenLandSectors::gen_seeds");
    note_result(seeds.m_n > 0u, "seed_n > 0");
    note_result(gls.sector_n() > 0u, "sector_n > 0");
    note_result(gls.paint_n() > 0u, "paint_n > 0");
    std::printf("gen_seeds: seeds=%u sectors=%u paint=%u lat=%u  wall_ms=%.3f\n",
        static_cast<unsigned>(seeds.m_n),
        static_cast<unsigned>(gls.sector_n()),
        static_cast<unsigned>(gls.paint_n()),
        25u,
        seeds_ms);

    t0 = std::chrono::steady_clock::now();
    const bool build_ok = gls.build(seeds);
    const double build_ms = ms_since(t0);
    note_result(build_ok, "GenLandSectors::build");
    std::printf("phase2 build: sectors=%u paint=%u  wall_ms=%.3f\n",
        static_cast<unsigned>(gls.sector_n()),
        static_cast<unsigned>(gls.paint_n()),
        build_ms);

    char pts_path[384];
    std::snprintf(pts_path, sizeof(pts_path), "%s/land_sector_points.ppm", G_OUT_DIR);
    note_result(write_points_ppm(pts_path, map, seeds), pts_path);
    std::printf("wrote %s\n", pts_path);

    if (seeds.m_n > 0u && seeds.m_pts != nullptr) {
        u32 tile_sum = 0u;
        u32 yield_sum = 0u;
        u32 res_sum = 0u;
        for (u16 i = 0; i < seeds.m_n; ++i) {
            tile_sum += seeds.m_pts[i].m_tiles;
            yield_sum += seeds.m_pts[i].m_yields;
            res_sum += seeds.m_pts[i].m_res;
        }
        std::printf("sector[0]: land=%u land_sec_n=%u tiles=%u yields=%u res=%u\n",
            static_cast<unsigned>(seeds.m_pts[0].m_land),
            static_cast<unsigned>(seeds.m_pts[0].m_land_sec_n),
            static_cast<unsigned>(seeds.m_pts[0].m_tiles),
            static_cast<unsigned>(seeds.m_pts[0].m_yields),
            static_cast<unsigned>(seeds.m_pts[0].m_res));
        std::printf("sums: tiles=%u yields=%u res=%u paint=%u\n",
            static_cast<unsigned>(tile_sum),
            static_cast<unsigned>(yield_sum),
            static_cast<unsigned>(res_sum),
            static_cast<unsigned>(gls.paint_n()));
        const size_t bytes = static_cast<size_t>(seeds.m_n) * sizeof(LandSectorSeedPt);
        std::printf("sector_array: n=%u sizeof(LandSectorSeedPt)=%zu bytes=%zu (%.3f KB)\n",
            static_cast<unsigned>(seeds.m_n),
            sizeof(LandSectorSeedPt),
            bytes,
            static_cast<double>(bytes) / 1024.0);
        note_result(tile_sum == gls.paint_n(), "tile_sum matches paint_n");
        note_result(yield_sum > 0u, "yield_sum > 0");
    }

    char sec_path[384];
    std::snprintf(sec_path, sizeof(sec_path), "%s/land_sectors.ppm", G_OUT_DIR);
    note_result(write_sectors_ppm(sec_path, map, gls), sec_path);
    std::printf("wrote %s\n", sec_path);

    char yld_path[384];
    char res_path[384];
    std::snprintf(yld_path, sizeof(yld_path), "%s/land_sector_yields.ppm", G_OUT_DIR);
    std::snprintf(res_path, sizeof(res_path), "%s/land_sector_resources.ppm", G_OUT_DIR);
    note_result(write_value_ppm(yld_path, map, gls, seeds, true), yld_path);
    note_result(write_value_ppm(res_path, map, gls, seeds, false), res_path);
    std::printf("wrote %s\n", yld_path);
    std::printf("wrote %s\n", res_path);

    GenLandSectors::free_seeds(&seeds);
    WhiteboardMng::terminate();
    std::printf("=======================================================\n");
    std::printf(" TESTING GEN_LAND_SECTORS: TOTAL FAILURES: %d/%d\n",
        total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
