//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "building_static_key.h"
#include "city.h"
#include "city_tile_manager.h"
#include "city_turn_handler.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-turn-handler-test";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 4;
static const u32 G_TURNS = 1000u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool build_paths () {
    char dir[256];
    if (std::snprintf(dir, sizeof(dir), "%s/p1-seed-%u", G_MAP_ROOT, G_SEED) <= 0) {
        return false;
    }
    if (std::snprintf(g_terr, sizeof(g_terr), "%s/terrain.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_clim, sizeof(g_clim), "%s/climate.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_riv, sizeof(g_riv), "%s/rivers.ppm", dir) <= 0) {
        return false;
    }
    if (std::snprintf(g_res, sizeof(g_res), "%s/resources.ppm", dir) <= 0) {
        return false;
    }
    return true;
}

static bool ensure_out_dir () {
    return ::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static void zpad (char* buf, size_t n, u32 v, int w) {
    std::snprintf(buf, n, "%0*u", w, v);
}

static void write_stats_hdr (std::FILE* f) {
    std::fprintf(f, "%-8s %-8s %-8s %-8s %-8s %-8s %-8s %-12s\n",
        "turn", "city", "pop", "food", "prod", "cult", "tiles", "commerce");
}

static void write_stats_row (
    std::FILE* f,
    u32 turn,
    u16 city_idx,
    const City& city,
    u32 tiles,
    u32 commerce
) {
    char t[16], c[16], p[16], fd[16], pr[16], cu[16], ti[16], cm[16];
    zpad(t, sizeof(t), turn, 8);
    zpad(c, sizeof(c), city_idx, 8);
    zpad(p, sizeof(p), city.get_current_population(), 8);
    zpad(fd, sizeof(fd), city.get_current_food_store(), 8);
    zpad(pr, sizeof(pr), city.get_current_production_store(), 8);
    zpad(cu, sizeof(cu), city.get_current_culture(), 8);
    zpad(ti, sizeof(ti), tiles, 8);
    zpad(cm, sizeof(cm), commerce, 12);
    std::fprintf(f, "%-8s %-8s %-8s %-8s %-8s %-8s %-8s %-12s\n", t, c, p, fd, pr, cu, ti, cm);
}

static void write_name_list (std::FILE* f, cstr label, cstr* names, u16 n) {
    std::fprintf(f, "  %s:", label);
    if (n == 0) {
        std::fprintf(f, " none\n");
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        const cstr nm = names[i] != nullptr ? names[i] : "?";
        if (i == 0) {
            std::fprintf(f, " %s", nm);
        } else {
            std::fprintf(f, ", %s", nm);
        }
    }
    std::fprintf(f, "\n");
}

static void write_builds (
    std::FILE* f,
    u32 turn,
    u16 city_idx,
    const City& city,
    const GameState& state
) {
    const RuntimeStatics& st = *state.m_statics;
    char t[16], c[16];
    zpad(t, sizeof(t), turn, 8);
    zpad(c, sizeof(c), city_idx, 8);
    std::fprintf(f, "turn=%-8s city=%-8s\n", t, c);

    cstr bld_nms[512];
    u16 bld_n = 0;
    const u16 bld_max = st.building().get_item_count();
    for (u16 i = 0; i < bld_max && bld_n < 512; ++i) {
        if (city.has_building(city_idx, i)) {
            bld_nms[bld_n] = st.building().get_name(BuildingStaticDataKey::from_raw(i));
            ++bld_n;
        }
    }
    write_name_list(f, "buildings", bld_nms, bld_n);

    cstr won_nms[512];
    u16 won_n = 0;
    const u16 won_max = st.wonder().get_item_count();
    for (u16 i = 0; i < won_max && won_n < 512; ++i) {
        if (state.m_wonder_city != nullptr && i < state.m_wonder_count && state.m_wonder_city[i] == city_idx) {
            won_nms[won_n] = st.wonder().get_name(WonderStaticDataKey::from_raw(i));
            ++won_n;
        }
    }
    write_name_list(f, "wonders", won_nms, won_n);

    cstr sw_nms[512];
    u16 sw_n = 0;
    const u16 owner = city.get_owner();
    const u16* sw_row = nullptr;
    if (owner < state.m_player_n && state.m_player_states != nullptr) {
        sw_row = state.m_player_states[owner].m_small_wonder_city;
    }
    const u16 sw_max = st.small_wonder().get_item_count();
    for (u16 i = 0; i < sw_max && sw_n < 512; ++i) {
        if (sw_row != nullptr && i < state.m_small_wonder_count && sw_row[i] == city_idx) {
            sw_nms[sw_n] = st.small_wonder().get_name(SmallWonderStaticDataKey::from_raw(i));
            ++sw_n;
        }
    }
    write_name_list(f, "small_wonders", sw_nms, sw_n);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("out dir failed\n");
        return 1;
    }
    char stats_path[256];
    char builds_path[256];
    std::snprintf(stats_path, sizeof(stats_path), "%s/city_turn_stats.log", G_OUT_DIR);
    std::snprintf(builds_path, sizeof(builds_path), "%s/city_turn_builds.log", G_OUT_DIR);
    std::FILE* stats_f = std::fopen(stats_path, "w");
    std::FILE* builds_f = std::fopen(builds_path, "w");
    if (stats_f == nullptr || builds_f == nullptr) {
        std::printf("log open failed\n");
        return 1;
    }
    write_stats_hdr(stats_f);

    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = nullptr;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        std::fclose(stats_f);
        std::fclose(builds_f);
        return 1;
    }
    const u16 city_n = state.m_cities.get_city_count();
    for (u16 i = 0; i < city_n; ++i) {
        City* city = state.m_cities.get_city(i);
        if (city == nullptr) {
            continue;
        }
        CityTileManager::maximize_food(city->get_owner(), i);
    }

    i64 handle_ns = 0;
    for (u32 turn = 0; turn < G_TURNS; ++turn) {
        for (u16 i = 0; i < city_n; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            CityTurnHandler::handle(state, i);
            const auto t1 = std::chrono::steady_clock::now();
            handle_ns += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        }
        for (u16 i = 0; i < city_n; ++i) {
            City* city = state.m_cities.get_city(i);
            if (city == nullptr) {
                continue;
            }
            const u16 owner = city->get_owner();
            u32 commerce = 0;
            if (owner < state.m_player_n && state.m_player_states != nullptr) {
                commerce = state.m_player_states[owner].m_commerce;
            }
            const u32 tiles = CityTileManager::count_worked(city->get_x(), city->get_y(), i);
            write_stats_row(stats_f, turn, i, *city, tiles, commerce);
            write_builds(builds_f, turn, i, *city, state);
        }
        state.m_current_turn = turn + 1;
    }

    std::fclose(stats_f);
    std::fclose(builds_f);
    state.clear();
    setup.release_map_gen();

    const f64 handle_ms = static_cast<f64>(handle_ns) / 1.0e6;
    const f64 per_call_ns = city_n > 0 ? static_cast<f64>(handle_ns) / static_cast<f64>(G_TURNS * static_cast<u32>(city_n)) : 0.0;
    std::printf("CityTurnHandler::handle total: %.3f ms over %u turns x %u cities\n", handle_ms, G_TURNS, city_n);
    std::printf("CityTurnHandler::handle mean: %.1f ns/call\n", per_call_ns);
    std::printf("stats: %s\n", stats_path);
    std::printf("builds: %s\n", builds_path);
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
