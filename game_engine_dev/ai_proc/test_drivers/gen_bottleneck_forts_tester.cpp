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
#include "gen_bottleneck_forts.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u8 G_RIV_R = 40u;
static const u8 G_RIV_G = 100u;
static const u8 G_RIV_B = 220u;
static const u32 G_MARK_A = 128u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
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
    return true;
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

static void base_map_rgb (const GameArraySimple& map, u16 x, u16 y, u8* r, u8* g, u8* b) {
    const u8 terr = map.get_terrain(x, y);
    terr_rgb(terr, r, g, b);
    if (!overlay_is_water_terr(terr) && map.get_river(x, y) != 0u) {
        *r = G_RIV_R;
        *g = G_RIV_G;
        *b = G_RIV_B;
    }
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

static void blend_rgb (u8* r, u8* g, u8* b, u8 tr, u8 tg, u8 tb, u32 a) {
    const u32 ia = 255u - a;
    *r = static_cast<u8>((ia * static_cast<u32>(*r) + a * static_cast<u32>(tr)) / 255u);
    *g = static_cast<u8>((ia * static_cast<u32>(*g) + a * static_cast<u32>(tg)) / 255u);
    *b = static_cast<u8>((ia * static_cast<u32>(*b) + a * static_cast<u32>(tb)) / 255u);
}

static bool write_fort_ppm (cstr path, const GameArraySimple& map, const GenBottleneckForts& gbf) {
    const Whiteboard_1B& mk = gbf.marks();
    const Whiteboard_1B& ft = gbf.forts();
    const u16 w = map.width();
    const u16 h = map.height();
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    const i32 zr = GenBottleneckForts::zoc_r();
    std::vector<u8> zoc(n, 0u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (ft.rd(x, y) == 0u) {
                continue;
            }
            for (i32 dy = -zr; dy <= zr; ++dy) {
                for (i32 dx = -zr; dx <= zr; ++dx) {
                    const i32 nx = static_cast<i32>(x) + dx;
                    const i32 ny = static_cast<i32>(y) + dy;
                    if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                        continue;
                    }
                    zoc[static_cast<u32>(ny) * static_cast<u32>(w) + static_cast<u32>(nx)] = 1u;
                }
            }
        }
    }
    std::vector<u8> rgb(static_cast<size_t>(n) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            base_map_rgb(map, x, y, &r, &g, &b);
            const u32 ti = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
            if (mk.rd(x, y) != 0u && zoc[ti] == 0u) {
                blend_rgb(&r, &g, &b, 0u, 0u, 255u, G_MARK_A);
            }
            if (zoc[ti] != 0u) {
                blend_rgb(&r, &g, &b, 255u, 0u, 0u, G_MARK_A);
            }
            if (ft.rd(x, y) != 0u) {
                r = 255u;
                g = 255u;
                b = 0u;
            }
            const u32 i = ti * 3u;
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

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    note_result(terr.width() > 0 && terr.height() > 0, "terrain size");
    const u16 mw = terr.width();
    const u16 mh = terr.height();

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");
    note_result(map.width() == mw && map.height() == mh, "map size matches");

    WhiteboardMng::init(mw, mh);
    note_result(WhiteboardMng::width() == mw && WhiteboardMng::height() == mh, "WhiteboardMng::init");

    GenBottleneckForts gbf;
    note_result(gbf.begin(map), "GenBottleneckForts::begin");

    auto t0 = std::chrono::steady_clock::now();
    const bool build_ok = gbf.build();
    const double build_ms = ms_since(t0);
    note_result(build_ok, "GenBottleneckForts::build");
    note_result(gbf.mark_n() > 0u, "mark_n > 0");
    note_result(gbf.fort_n() > 0u, "fort_n > 0");
    std::printf("build: mark_sectors=%u forts=%u  wall_ms=%.3f\n",
        gbf.mark_n(), gbf.fort_n(), build_ms);

    char ppm_path[384];
    std::snprintf(ppm_path, sizeof(ppm_path), "%s/gen_bottleneck_forts.ppm", g_dir);
    note_result(write_fort_ppm(ppm_path, map, gbf), ppm_path);
    std::printf("wrote %s\n", ppm_path);

    WhiteboardMng::terminate();

    std::printf("=======================================================\n");
    std::printf(" GEN BOTTLENECK FORTS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
