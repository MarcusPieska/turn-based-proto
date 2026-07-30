//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

#include "bit_array.h"
#include "building_static_key.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_tile_manager.h"
#include "city_turn_handler.h"
#include "circular_tile_areas.h"
#include "combat_mods.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "general_assessor.h"
#include "general_bit_bank.h"
#include "linear_tech.h"
#include "runtime_statics.h"
#include "starting_point_generator.h"
#include "tech_static_data.h"
#include "tech_static_key.h"
#include "tile_yields.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/single-city-dev-test";
static const u32 G_SEED = 43u;
static const u32 G_TURNS = 1000u;
static const u16 G_LATT_STEP = 20u;
static const u16 G_LATT_ORIG = 10u;
static const i32 G_SCAN_R = 7;
static const u16 G_PICK_N = 10u;
static const u16 G_REACH_R = 4u;
static const u16 G_CLAIM_CULT = 25u;
static const u32 G_SITE_MAX = 4096u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

//================================================================================================================================
//=> - Site -
//================================================================================================================================

struct Site {
    u16 m_x;
    u16 m_y;
    u32 m_food;
};

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
    if (std::snprintf(g_ov, sizeof(g_ov), "%s/overlay.ppm", dir) <= 0) {
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

static bool is_water (u8 terr) {
    return terr == TERR_OCEAN[0]
        || terr == TERR_SEA[0]
        || terr == TERR_COASTAL[0]
        || terr == TERR_INLAND_SEA[0]
        || terr == TERR_INLAND_LAKE[0];
}

static bool is_mountain (u8 terr) {
    return terr == TERR_MOUNTAINS[0] || terr == TERR_VOLCANO[0];
}

static bool tile_ok (const GameArraySimple& map, u16 x, u16 y) {
    if (x >= map.width() || y >= map.height()) {
        return false;
    }
    const u8 terr = map.get_terrain(x, y);
    if (is_water(terr) || is_mountain(terr)) {
        return false;
    }
    if (map.get_climate(x, y) == CLIMATE_DESERT) {
        return false;
    }
    return true;
}

static bool find_near (const GameArraySimple& map, u16 lx, u16 ly, u16* ox, u16* oy) {
    if (tile_ok(map, lx, ly)) {
        *ox = lx;
        *oy = ly;
        return true;
    }
    bool found = false;
    i32 best = 0;
    for (i32 dy = -G_SCAN_R; dy <= G_SCAN_R; ++dy) {
        for (i32 dx = -G_SCAN_R; dx <= G_SCAN_R; ++dx) {
            const i32 x = static_cast<i32>(lx) + dx;
            const i32 y = static_cast<i32>(ly) + dy;
            if (x < 0 || y < 0) {
                continue;
            }
            if (static_cast<u16>(x) >= map.width() || static_cast<u16>(y) >= map.height()) {
                continue;
            }
            if (!tile_ok(map, static_cast<u16>(x), static_cast<u16>(y))) {
                continue;
            }
            const i32 d = dx * dx + dy * dy;
            if (!found || d < best) {
                best = d;
                *ox = static_cast<u16>(x);
                *oy = static_cast<u16>(y);
                found = true;
            }
        }
    }
    return found;
}

static u32 site_plains_food (const GameArraySimple& map, u16 cx, u16 cy) {
    u32 food = 0;
    if (map.get_terrain(cx, cy) == TERR_PLAINS[0]) {
        food = food + static_cast<u32>(TileYields::get(cx, cy).m_food);
    }
    const CircArea area = CircularTileAreas::get(G_REACH_R);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= map.width() || uy >= map.height()) {
            continue;
        }
        if (map.get_terrain(ux, uy) != TERR_PLAINS[0]) {
            continue;
        }
        food = food + static_cast<u32>(TileYields::get(ux, uy).m_food);
    }
    return food;
}

static int site_cmp (const void* a, const void* b) {
    const Site* sa = static_cast<const Site*>(a);
    const Site* sb = static_cast<const Site*>(b);
    if (sa->m_food < sb->m_food) {
        return -1;
    }
    if (sa->m_food > sb->m_food) {
        return 1;
    }
    return 0;
}

static u32 collect_sites (const GameArraySimple& map, Site* out, u32 out_max) {
    u32 n = 0;
    for (u16 y = G_LATT_ORIG; y < map.height(); y = static_cast<u16>(y + G_LATT_STEP)) {
        for (u16 x = G_LATT_ORIG; x < map.width(); x = static_cast<u16>(x + G_LATT_STEP)) {
            u16 sx = 0;
            u16 sy = 0;
            if (!find_near(map, x, y, &sx, &sy)) {
                continue;
            }
            if (n >= out_max) {
                return n;
            }
            out[n].m_x = sx;
            out[n].m_y = sy;
            out[n].m_food = site_plains_food(map, sx, sy);
            n = n + 1u;
        }
    }
    return n;
}

static void pick_even (const Site* sorted, u32 n, Site* picks, u16 pick_n) {
    if (n == 0 || pick_n == 0) {
        return;
    }
    if (n == 1 || pick_n == 1) {
        picks[0] = sorted[0];
        return;
    }
    const u16 lim = (pick_n < static_cast<u16>(n)) ? pick_n : static_cast<u16>(n);
    for (u16 i = 0; i < lim; ++i) {
        const u32 idx = (i * (n - 1u)) / static_cast<u32>(lim - 1u);
        picks[i] = sorted[idx];
    }
}

static void grant_all_resources (GameState& state, u16 city_idx) {
    GeneralBitBank* bank = state.m_cities.get_res_bank();
    if (bank == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 res_n = state.m_statics->resource().get_item_count();
    for (u16 i = 0; i < res_n; ++i) {
        bank->set_flag(city_idx, i);
    }
}

static void clr_owned (BitArrayCL& available, const BitArrayCL& owned) {
    const u32 n = available.get_count();
    for (u32 i = 0; i < n; ++i) {
        if (owned.get_bit(i) != 0) {
            available.clear_bit(i);
        }
    }
}

static bool unlock_one_tech (GameState& state, u16 player, u16* out_idx) {
    if (state.m_statics == nullptr || state.m_player_states == nullptr || player >= state.m_player_n) {
        return false;
    }
    PlayerState& ps = state.m_player_states[player];
    const RuntimeStatics& st = *state.m_statics;
    const u16 tech_n = st.tech().get_item_count();
    if (tech_n == 0) {
        return false;
    }
    if (ps.m_techs_researched == nullptr) {
        ps.m_techs_researched = new BitArrayCL(tech_n);
    }
    if (ps.m_techs_researched == nullptr) {
        return false;
    }
    BitArrayCL resource(st.resource().get_item_count());
    BitArrayCL building(st.building().get_item_count());
    BitArrayCL city_flag(st.city_flag().get_item_count());
    BitArrayCL civ(st.civ().get_item_count());
    for (u32 i = 0; i < resource.get_count(); ++i) {
        resource.set_bit(i);
    }
    if (ps.m_civ_index < civ.get_count()) {
        civ.set_bit(ps.m_civ_index);
    }
    AssessorCtx ctx = {};
    ctx.m_tech = ps.m_techs_researched;
    ctx.m_civ = &civ;
    ctx.m_city_idx = 0;
    ctx.m_resource = &resource;
    ctx.m_building = &building;
    ctx.m_city_flag = &city_flag;
    BitArrayCL available(tech_n);
    const TechStaticDataStruct* items = &st.tech().get_item(TechStaticDataKey::from_raw(0));
    GeneralAssessor::assess_tech(&available, tech_n, items, ctx);
    clr_owned(available, *ps.m_techs_researched);
    u16 pick = U16_KEY_NULL;
    if (!LinearTech::pick(available, &pick)) {
        return false;
    }
    ps.m_techs_researched->set_bit(pick);
    if (out_idx != nullptr) {
        *out_idx = pick;
    }
    return true;
}

static void claim_start_borders (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        CityBorder::claim_expand(c->get_x(), c->get_y(), 0, G_CLAIM_CULT, static_cast<u8>(c->get_owner()));
    }
}

static void trace_new_buildings (
    std::FILE* f,
    u32 turn,
    GameState& state,
    u16 city_idx,
    City& city,
    BitArrayCL& seen) 
{
    const RuntimeStatics& st = *state.m_statics;
    const u16 bld_n = st.building().get_item_count();
    for (u16 i = 0; i < bld_n; ++i) {
        if (!city.has_building(city_idx, i)) {
            continue;
        }
        if (seen.get_bit(i) != 0) {
            continue;
        }
        seen.set_bit(i);
        cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        std::fprintf(f, "turn=%08u building=%s\n", turn, nm != nullptr ? nm : "?");
    }
}

static bool boot_at_site (
    GameSetup& setup,
    GameState& state,
    const RuntimeStatics* st,
    u16 x,
    u16 y) 
{
    state.clear();
    state.m_statics = st;
    state.m_civ_relations.reset(st->civ().get_item_count());
    if (!state.m_combat_mods.setup(*st)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_map_gen_data(&state.m_map, g_terr, g_clim, g_riv, g_ov)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state.m_map, g_res)) {
        return false;
    }
    SpgPickCoords starts = {};
    starts.n = 1;
    starts.pts[0].x = x;
    starts.pts[0].y = y;
    return setup.finish_with_starts(&state, starts, 1);
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

    GameSetup setup;
    GameState probe;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&probe, paths, 1)) {
        std::printf("probe setup failed\n");
        return 1;
    }
    const RuntimeStatics* st = probe.m_statics;
    if (st == nullptr) {
        std::printf("statics null\n");
        probe.clear();
        return 1;
    }

    Site sites[G_SITE_MAX];
    const u32 site_n = collect_sites(probe.m_map, sites, G_SITE_MAX);
    if (site_n == 0) {
        std::printf("no viable lattice sites\n");
        probe.clear();
        return 1;
    }
    std::qsort(sites, site_n, sizeof(Site), site_cmp);
    Site picks[G_PICK_N];
    const u16 pick_n = (site_n < G_PICK_N) ? static_cast<u16>(site_n) : G_PICK_N;
    pick_even(sites, site_n, picks, pick_n);
    std::printf("sites collected=%u picked=%u\n", site_n, static_cast<u32>(pick_n));
    for (u16 i = 0; i < pick_n; ++i) {
        std::printf("  pick %u: (%u,%u) plains_food=%u\n", static_cast<u32>(i), picks[i].m_x, picks[i].m_y, picks[i].m_food);
    }
    probe.clear();

    const u16 tech_n = st->tech().get_item_count();
    const u16 bld_n = st->building().get_item_count();
    i64 city_ns[G_PICK_N];
    u32 city_calls[G_PICK_N];
    u16 city_end_pop[G_PICK_N];
    for (u16 i = 0; i < G_PICK_N; ++i) {
        city_ns[i] = 0;
        city_calls[i] = 0;
        city_end_pop[i] = 0;
    }

    GameState state;
    for (u16 pi = 0; pi < pick_n; ++pi) {
        if (!boot_at_site(setup, state, st, picks[pi].m_x, picks[pi].m_y)) {
            std::printf("boot failed for pick %u\n", static_cast<u32>(pi));
            state.clear();
            return 1;
        }
        const u16 city_idx = 0;
        City* city = state.m_cities.get_city(city_idx);
        if (city == nullptr) {
            std::printf("city null for pick %u\n", static_cast<u32>(pi));
            state.clear();
            return 1;
        }
        grant_all_resources(state, city_idx);
        claim_start_borders(state);
        CityTileManager::maximize_food(city->get_owner(), city_idx);

        char path[320];
        std::snprintf(path, sizeof(path), "%s/%03u.log", G_OUT_DIR, static_cast<u32>(pi));
        std::FILE* f = std::fopen(path, "w");
        if (f == nullptr) {
            std::printf("trace open failed: %s\n", path);
            state.clear();
            return 1;
        }
        std::fprintf(f, "# pick=%u x=%u y=%u food=%u\n", static_cast<u32>(pi), picks[pi].m_x, picks[pi].m_y, picks[pi].m_food);

        BitArrayCL seen_bld(bld_n);
        u32 unlocked = 0;
        bool have_prev = false;
        u16 prev_pop = 0;
        i16 prev_net = 0;
        u16 prev_boost = 0;
        for (u32 turn = 0; turn < G_TURNS; ++turn) {
            const auto t0 = std::chrono::steady_clock::now();
            CityTurnHandler::handle(state, city_idx);
            const auto t1 = std::chrono::steady_clock::now();
            city_ns[pi] += std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
            city_calls[pi] = city_calls[pi] + 1u;

            const u16 pop = city->get_current_population();
            const u16 boost = city->get_city_sanitation_boost(city_idx);
            const i16 net = city->get_city_net_sanitation(boost);
            if (!have_prev || pop != prev_pop || boost != prev_boost || net != prev_net) {
                std::fprintf(f, "turn=%08u pop=%u sanitation=%d sanitation_boost=%u\n",
                    turn, static_cast<u32>(pop), static_cast<int>(net), static_cast<u32>(boost));
            }
            prev_pop = pop;
            prev_boost = boost;
            prev_net = net;
            have_prev = true;
            trace_new_buildings(f, turn, state, city_idx, *city, seen_bld);

            u32 target = 0;
            if (tech_n > 0) {
                target = ((turn + 1u) * static_cast<u32>(tech_n)) / G_TURNS;
            }
            while (unlocked < target) {
                u16 tech_idx = U16_KEY_NULL;
                if (!unlock_one_tech(state, city->get_owner(), &tech_idx)) {
                    break;
                }
                cstr nm = st->tech().get_name(TechStaticDataKey::from_raw(tech_idx));
                std::fprintf(f, "turn=%08u tech=%s\n", turn, nm != nullptr ? nm : "?");
                unlocked = unlocked + 1u;
            }
            state.m_current_turn = turn + 1;
        }
        city_end_pop[pi] = city->get_current_population();
        std::fclose(f);
        std::printf("wrote %s\n", path);
        state.clear();
    }

    i64 handle_ns = 0;
    u32 handle_n = 0;
    std::printf("CityTurnHandler::handle by city:\n");
    for (u16 pi = 0; pi < pick_n; ++pi) {
        handle_ns += city_ns[pi];
        handle_n += city_calls[pi];
        const f64 total_ms = static_cast<f64>(city_ns[pi]) / 1.0e6;
        const f64 avg_us = (city_calls[pi] == 0) ? 0.0
            : (static_cast<f64>(city_ns[pi]) / 1.0e3) / static_cast<f64>(city_calls[pi]);
        std::printf("  pick %u end_pop=%u avg: %.2f us  total: %.2f ms\n",
            static_cast<u32>(pi), static_cast<u32>(city_end_pop[pi]), avg_us, total_ms);
    }
    const f64 total_ms = static_cast<f64>(handle_ns) / 1.0e6;
    const f64 avg_us = (handle_n == 0) ? 0.0 : (static_cast<f64>(handle_ns) / 1.0e3) / static_cast<f64>(handle_n);
    std::printf("CityTurnHandler::handle all avg: %.2f us\n", avg_us);
    std::printf("CityTurnHandler::handle all total: %.2f ms\n", total_ms);
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
