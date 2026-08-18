//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "gen_settlement_targets.h"
#include "map_loader.h"
#include "map_terrain_data.h"
#include "starting_point_generator.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const u32 G_SEED = 43u;
static const u16 G_PICK_N = 10u;
static const u16 G_LATT_DIV = 10u;
static const u8 G_OWNER = 0u;
static const u16 G_FULL_CULT = 50000u;
static const int G_DOT_R = 0;
static const u8 G_OWN_A = 96u;
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

static void latt_for_map (u16 w, u16 h, u16* rows, u16* cols) {
    u16 r = h / G_LATT_DIV;
    u16 c = w / G_LATT_DIV;
    if (r == 0) {
        r = 1;
    }
    if (c == 0) {
        c = 1;
    }
    *rows = r;
    *cols = c;
}

static void fill_clim_riv_mtn (std::vector<u8>& rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(map.get_climate(x, y), &r, &g, &b);
            if (map.get_river(x, y) != 0u) {
                r = G_RIV_R;
                g = G_RIV_G;
                b = G_RIV_B;
            }
            if (map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
}

static u8 blend_u8 (u8 base, u8 over, u8 a) {
    const u16 inv = static_cast<u16>(255u - a);
    return static_cast<u8>((static_cast<u16>(base) * inv + static_cast<u16>(over) * a) / 255u);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 cx, u16 cy) {
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
            rgb[i] = 220;
            rgb[i + 1] = 30;
            rgb[i + 2] = 30;
        }
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

static void paint_riv_mtn (std::vector<u8>& rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            if (map.get_river(x, y) != 0u) {
                rgb[i] = G_RIV_R;
                rgb[i + 1] = G_RIV_G;
                rgb[i + 2] = G_RIV_B;
            }
            if (map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                rgb[i] = 120;
                rgb[i + 1] = 72;
                rgb[i + 2] = 40;
            }
        }
    }
}

static void paint_planned (std::vector<u8>& rgb, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_planned_city(x, y) != 0u) {
                paint_dot(rgb, w, h, x, y);
            }
        }
    }
}

static bool write_planned_ppm (cstr path, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_clim_riv_mtn(rgb, map);
    paint_planned(rgb, map);
    return write_ppm(path, rgb, w, h);
}

static bool write_own_ppm (cstr path, const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    fill_clim_riv_mtn(rgb, map);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (overlay_is_water_terr(map.get_terrain(x, y))) {
                continue;
            }
            if (map.get_civ_owner(x, y) != G_OWNER) {
                continue;
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = blend_u8(rgb[i], 220u, G_OWN_A);
            rgb[i + 1] = blend_u8(rgb[i + 1], 30u, G_OWN_A);
            rgb[i + 2] = blend_u8(rgb[i + 2], 30u, G_OWN_A);
        }
    }
    paint_riv_mtn(rgb, map);
    paint_planned(rgb, map);
    return write_ppm(path, rgb, w, h);
}

static void land_own_pct (const GameArraySimple& map, u32* land_n, u32* own_n, double* pct) {
    u32 land = 0;
    u32 own = 0;
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (overlay_is_water_terr(map.get_terrain(x, y))) {
                continue;
            }
            ++land;
            if (map.get_civ_owner(x, y) == G_OWNER) {
                ++own;
            }
        }
    }
    *land_n = land;
    *own_n = own;
    *pct = land == 0u ? 0.0 : (100.0 * static_cast<double>(own)) / static_cast<double>(land);
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
    GameArraySimple map;
    note_result(Factory_GameArraySimple::load_map_gen_data(&map, g_terr, g_clim, g_riv, g_ov), "load map");

    const u16 mw = map.width();
    const u16 mh = map.height();
    const u32 tn = map.tile_n();
    std::vector<u8> clim(tn);
    std::vector<u8> ov(tn);
    std::vector<u8> riv(tn);
    for (u16 y = 0; y < mh; ++y) {
        for (u16 x = 0; x < mw; ++x) {
            const u32 i = static_cast<u32>(y) * static_cast<u32>(mw) + static_cast<u32>(x);
            clim[i] = map.get_climate(x, y);
            ov[i] = map.get_overlay(x, y);
            riv[i] = map.get_river(x, y);
        }
    }
    u16 latt_r = 0;
    u16 latt_c = 0;
    latt_for_map(mw, mh, &latt_r, &latt_c);
    StartingPointGeneratorParams par = {};
    par.map = &terr;
    par.climate = clim.data();
    par.overlay = ov.data();
    par.river = riv.data();
    par.pick_n = G_PICK_N;
    par.latt_rows = latt_r;
    par.latt_cols = latt_c;
    par.seed = G_SEED;
    StartingPointGenerator spg(par);
    note_result(spg.generate(), "generate starting points");
    const SpgPickCoords starts = spg.picks_coords();
    note_result(starts.n > 0u, "have starting points");
    std::printf("starts=%u map=%ux%u\n", starts.n, map.width(), map.height());

    GenSettlementTargetsRslt rslt = {};
    const auto t0 = std::chrono::steady_clock::now();
    const bool gen_ok = GenSettlementTargets::generate(map, starts.pts, starts.n, &rslt);
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    note_result(gen_ok, "GenSettlementTargets::generate");
    std::printf(
        "planned total=%u seed=%u river=%u pack=%u  wall_ms=%.3f\n",
        rslt.m_n,
        rslt.m_seed_n,
        rslt.m_river_n,
        rslt.m_pack_n,
        ms);
    note_result(rslt.m_n > 0u, "planned sites > 0");

    char ppm_plan[384];
    std::snprintf(ppm_plan, sizeof(ppm_plan), "%s/gen_settlement_targets.ppm", g_dir);
    note_result(write_planned_ppm(ppm_plan, map), ppm_plan);
    std::printf("wrote %s\n", ppm_plan);

    CityArray cities;
    CityBorder::bind_map(&map);
    note_result(CityBorder::get(20u).m_lim > 0u, "border radius supports 20");

    u32 founded = 0;
    for (u16 y = 0; y < map.height(); ++y) {
        for (u16 x = 0; x < map.width(); ++x) {
            if (map.get_planned_city(x, y) == 0u) {
                continue;
            }
            const u16 city_idx = cities.get_next_new_city_idx();
            City* city = cities.get_city(city_idx);
            if (city == nullptr) {
                continue;
            }
            city->init(G_OWNER, x, y);
            map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY);
            ++founded;
        }
    }
    note_result(founded == rslt.m_n, "founded cities == planned");
    std::printf("founded_cities=%u\n", founded);

    for (u16 i = 0; i < cities.get_city_count(); ++i) {
        City* city = cities.get_city(i);
        if (city == nullptr) {
            continue;
        }
        CityBorder::claim_expand(city->get_x(), city->get_y(), 0u, G_FULL_CULT, G_OWNER);
    }

    u32 land_n = 0;
    u32 own_n = 0;
    double pct = 0.0;
    land_own_pct(map, &land_n, &own_n, &pct);
    std::printf("land=%u owned=%u ownership=%.2f%%\n", land_n, own_n, pct);
    note_result(own_n > 0u, "owned land > 0");

    char ppm_own[384];
    std::snprintf(ppm_own, sizeof(ppm_own), "%s/gen_settlement_targets_own.ppm", g_dir);
    note_result(write_own_ppm(ppm_own, map), ppm_own);
    std::printf("wrote %s\n", ppm_own);

    CityBorder::bind_map(nullptr);
    std::printf("=======================================================\n");
    std::printf(" GEN SETTLEMENT TARGETS: TOTAL FAILURES: %d/%d\n", total_test_fails, total_tests_run);
    std::printf("=======================================================\n");
    return total_test_fails > 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
