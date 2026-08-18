//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "resource_static_key.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "worker_job_enum.h"
#include "worker_job_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const int G_DOT_R = 1;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_dir[256];
static char g_ppm[384];

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
    if (std::snprintf(g_ppm, sizeof(g_ppm), "%s/aux_res_imp_terrain_bad.ppm", g_dir) <= 0) {
        return false;
    }
    return true;
}

static bool is_water (u8 terr) {
    return overlay_is_water_terr(terr);
}

static bool is_mountain (u8 terr) {
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* const k_rows[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE, TERR_TILE_SENTINEL
    };
    for (u32 i = 0; i < sizeof(k_rows) / sizeof(k_rows[0]); ++i) {
        if (k_rows[i][0] == terr) {
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

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 cx, u16 cy, u8 r, u8 g, u8 b) {
    for (int dy = -G_DOT_R; dy <= G_DOT_R; ++dy) {
        for (int dx = -G_DOT_R; dx <= G_DOT_R; ++dx) {
            if (dx * dx + dy * dy > G_DOT_R * G_DOT_R) {
                continue;
            }
            const int x = static_cast<int>(cx) + dx;
            const int y = static_cast<int>(cy) + dy;
            if (x < 0 || y < 0 || x >= static_cast<int>(w) || y >= static_cast<int>(h)) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static bool write_ppm (cstr path, const GameArraySimple& map, const std::vector<u8>& kind) {
    FILE* f = std::fopen(path, "wb");
    if (f == nullptr) {
        return false;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(map.get_terrain(x, y), &r, &g, &b);
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 k = kind[static_cast<u32>(y) * w + x];
            if (k == 1u) {
                paint_dot(rgb, w, h, x, y, 220, 30, 30);
            } else if (k == 2u) {
                paint_dot(rgb, w, h, x, y, 160, 40, 200);
            }
        }
    }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    const size_t nbytes = rgb.size();
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, f) == nbytes;
    std::fclose(f);
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char* argv[]) {
    if (argc > 1) {
        print_level = std::atoi(argv[1]);
    }
    note_result(build_paths(), "build map paths");

    RuntimeStaticLoader loader;
    note_result(loader.load(G_RT_LIB, G_RT_DATA), "load runtime statics");
    if (total_test_fails > 0) {
        std::printf("=======================================================\n");
        std::printf(" AUX RES IMP TERRAIN: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
        std::printf("=======================================================\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(Factory_GameArraySimple::load_res_dist_data(&map, g_res), "load resources");

    const u16 mine_job = static_cast<u16>(WorkerJob::Mine);
    const u16 plant_job = static_cast<u16>(WorkerJob::Plantation);
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> kind(static_cast<size_t>(w) * static_cast<size_t>(h), 0u);
    u32 plant_bad = 0;
    u32 mine_bad = 0;

    std::printf("-----------------------------------------------------------\n");
    std::printf("AUX: plantation on water/mountain, mine on water\n");
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 ri = map.get_res(x, y);
            if (ri == U16_KEY_NULL) {
                continue;
            }
            if (ri >= st.resource().get_item_count()) {
                continue;
            }
            const ResourceStaticDataKey key = ResourceStaticDataKey::from_raw(ri);
            const ResourceStaticDataStruct& row = st.resource().get_item(key);
            const u8 terr = map.get_terrain(x, y);
            const bool water = is_water(terr);
            const bool mtn = is_mountain(terr);
            cstr nm = st.resource().get_name(key);

            if (row.worker_job_idx == plant_job && (water || mtn)) {
                ++plant_bad;
                kind[static_cast<u32>(y) * w + x] = 1u;
                if (print_level > 0) {
                    std::printf("  Plantation %s @(%u,%u) on %s\n", nm, x, y, water ? "water" : "mountain");
                }
            }
            if (row.worker_job_idx == mine_job && water) {
                ++mine_bad;
                kind[static_cast<u32>(y) * w + x] = 2u;
                if (print_level > 0) {
                    std::printf("  Mine %s @(%u,%u) on water\n", nm, x, y);
                }
            }
        }
    }
    std::printf("  summary plantation_bad=%u mine_bad=%u\n", plant_bad, mine_bad);
    note_result(write_ppm(g_ppm, map, kind), "write aux_res_imp_terrain_bad.ppm");
    std::printf("  wrote %s\n", g_ppm);
    if (print_level > 0) {
        std::printf("  (report only; plantation_bad=%u mine_bad=%u)\n", plant_bad, mine_bad);
    }
    loader.unload();
    std::printf("=======================================================\n");
    std::printf(" AUX RES IMP TERRAIN: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
