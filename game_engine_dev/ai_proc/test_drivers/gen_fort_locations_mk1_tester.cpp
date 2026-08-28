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
#include "gen_fort_locations_mk1.h"
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

static void patch_rgb (u16 id, u8* r, u8* g, u8* b) {
    *r = static_cast<u8>(180u + ((static_cast<u32>(id) * 37u) % 75u));
    *g = static_cast<u8>(180u + ((static_cast<u32>(id) * 61u) % 75u));
    *b = static_cast<u8>(180u + ((static_cast<u32>(id) * 91u) % 75u));
}

static void frag_rgb (u16 id, u8* r, u8* g, u8* b) {
    static const u8 k_cols[3][3] = {
        {220u, 40u, 40u},
        {160u, 40u, 200u},
        {255u, 105u, 180u}};
    const u32 k = static_cast<u32>((id - 1u) % 3u);
    *r = k_cols[k][0];
    *g = k_cols[k][1];
    *b = k_cols[k][2];
}

static void walk_rgb (u16 step, u16 max_c, u8* r, u8* g, u8* b) {
    if (max_c <= 1u) {
        *r = 40u;
        *g = 40u;
        *b = 220u;
        return;
    }
    const u32 t = static_cast<u32>(step - 1u);
    const u32 d = static_cast<u32>(max_c - 1u);
    *r = static_cast<u8>((40u * (d - t) + 220u * t) / d);
    *g = 40u;
    *b = static_cast<u8>((220u * (d - t) + 40u * t) / d);
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

static bool write_patch_ppm (cstr path, const GenFortLocationsMk1& gfl) {
    const Whiteboard_2B& ov = gfl.patches();
    const u16 w = ov.w();
    const u16 h = ov.h();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u, 0u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u16 id = ov.rd(x, y);
            if (id == GFL_IDX_NONE) {
                continue;
            }
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            patch_rgb(id, &r, &g, &b);
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_frag_ppm (cstr path, const GameArraySimple& map, const GenFortLocationsMk1& gfl) {
    const Whiteboard_2B& ov = gfl.frags();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            base_map_rgb(map, x, y, &r, &g, &b);
            const u16 id = ov.rd(x, y);
            if (id != GFL_IDX_NONE) {
                frag_rgb(id, &r, &g, &b);
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_walk_ppm (cstr path, const GameArraySimple& map, const GenFortLocationsMk1& gfl) {
    const Whiteboard_2B& walk = gfl.walks();
    const Whiteboard_2B& land = gfl.frags();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            base_map_rgb(map, x, y, &r, &g, &b);
            const u16 step = walk.rd(x, y);
            const u16 fid = land.rd(x, y);
            const GflFragInfo* info = gfl.frag_info(fid);
            if (step != 0u && info != nullptr && info->m_steps > 0u) {
                walk_rgb(step, info->m_steps, &r, &g, &b);
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    return write_ppm(path, rgb, w, h);
}

static bool write_pass_ppm (cstr path, const GameArraySimple& map, const GenFortLocationsMk1& gfl) {
    const Whiteboard_1B& pass = gfl.passes();
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            base_map_rgb(map, x, y, &r, &g, &b);
            if (pass.rd(x, y) != 0u) {
                r = 255u;
                g = 0u;
                b = 0u;
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

    MapTerrainData terr;
    note_result(MapLoader::load_terrain_ppm(g_terr, terr), "load terrain");
    note_result(terr.width() > 0 && terr.height() > 0, "terrain size");
    const u16 mw = terr.width();
    const u16 mh = terr.height();

    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");
    note_result(map.width() == mw && map.height() == mh, "map size matches");

    WhiteboardMng::init(mw, mh);
    note_result(WhiteboardMng::width() == mw && WhiteboardMng::height() == mh, "WhiteboardMng::init");

    GenFortLocationsMk1 gfl;
    note_result(gfl.begin(map), "GenFortLocationsMk1::begin");

    auto t0 = std::chrono::steady_clock::now();
    const bool idx_ok = gfl.index_patches(map);
    const double idx_ms = ms_since(t0);
    note_result(idx_ok, "GenFortLocationsMk1::index_patches");
    note_result(gfl.patch_n() > 0u, "patch_n > 0");
    std::printf("index_patches: patches=%u  wall_ms=%.3f\n", gfl.patch_n(), idx_ms);

    t0 = std::chrono::steady_clock::now();
    const bool pass_ok = gfl.find_passes(map);
    const double pass_ms = ms_since(t0);
    note_result(pass_ok, "GenFortLocationsMk1::find_passes");
    note_result(gfl.pass_n() > 0u, "pass_n > 0");
    note_result(gfl.frag_n() > 0u, "frag_n > 0");
    u32 loop_n = 0;
    u32 open_n = 0;
    u16 max_steps = 0;
    for (u16 f = 1u; f <= gfl.frag_n(); ++f) {
        const GflFragInfo* info = gfl.frag_info(f);
        if (info == nullptr) {
            continue;
        }
        if (info->m_loop != 0u) {
            ++loop_n;
        } else {
            ++open_n;
        }
        if (info->m_steps > max_steps) {
            max_steps = info->m_steps;
        }
    }
    note_result(max_steps > 0u, "walk max_steps > 0");
    std::printf("find_passes: pass_tiles=%u frags=%u open=%u loop=%u max_steps=%u  wall_ms=%.3f\n",
        gfl.pass_n(), gfl.frag_n(), open_n, loop_n, max_steps, pass_ms);

    char ppm_patch[384];
    std::snprintf(ppm_patch, sizeof(ppm_patch), "%s/gen_fort_locations_mk1_patches.ppm", g_dir);
    note_result(write_patch_ppm(ppm_patch, gfl), ppm_patch);
    std::printf("wrote %s\n", ppm_patch);

    char ppm_frag[384];
    std::snprintf(ppm_frag, sizeof(ppm_frag), "%s/gen_fort_locations_mk1_frags.ppm", g_dir);
    note_result(write_frag_ppm(ppm_frag, map, gfl), ppm_frag);
    std::printf("wrote %s\n", ppm_frag);

    char ppm_walk[384];
    std::snprintf(ppm_walk, sizeof(ppm_walk), "%s/gen_fort_locations_mk1_walks.ppm", g_dir);
    note_result(write_walk_ppm(ppm_walk, map, gfl), ppm_walk);
    std::printf("wrote %s\n", ppm_walk);

    char ppm_pass[384];
    std::snprintf(ppm_pass, sizeof(ppm_pass), "%s/gen_fort_locations_mk1_passes.ppm", g_dir);
    note_result(write_pass_ppm(ppm_pass, map, gfl), ppm_pass);
    std::printf("wrote %s\n", ppm_pass);

    WhiteboardMng::terminate();

    std::printf("=======================================================\n");
    std::printf(" GEN FORT LOCATIONS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails == 0 ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
