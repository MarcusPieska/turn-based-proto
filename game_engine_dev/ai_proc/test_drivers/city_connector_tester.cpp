//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_connector.h"
#include "city_tile_manager.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "map_bit_overlay.h"
#include "runtime_static_loader.h"
#include "unit_add_struct.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "worker_helper.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

static const char* G_RT_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "../../";
static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/city-connector-test";
static const u32 G_SEED = 43u;
static const u16 G_CITY_MAX = 5000;
static const u16 G_WORKER_MAX = 5000;
static const u32 G_TURNS = 100u;
static const u32 G_IMG_EVERY = 1u;
static const u16 G_EDGE_PAD = 4;
static const u16 G_DIST_BLACK = 4;
static const u16 G_DIST_GRASS = 8;
static const u16 G_DIST_PLAINS = 12;
static const u16 G_CLAIM_CULT = 100u;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_res[320];

static u16 g_cx[G_CITY_MAX];
static u16 g_cy[G_CITY_MAX];
static u16 g_wk[G_WORKER_MAX];
static u16 g_city_n = 0;
static u16 g_wk_n = 0;

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
    return mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST;
}

static bool is_city_terr (u8 terr) {
    return terr == TERR_PLAINS[0] || terr == TERR_HILLS[0];
}

static bool is_city_clim (u8 clim) {
    return clim == CLIMATE_GRASSLAND || clim == CLIMATE_PLAINS || clim == CLIMATE_BLACK_SOIL;
}

static u16 clim_min_dist (u8 clim) {
    if (clim == CLIMATE_BLACK_SOIL) {
        return G_DIST_BLACK;
    }
    if (clim == CLIMATE_GRASSLAND) {
        return G_DIST_GRASS;
    }
    if (clim == CLIMATE_PLAINS) {
        return G_DIST_PLAINS;
    }
    return G_DIST_PLAINS;
}

static u16 cheb_dist (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(ax) - static_cast<i32>(bx);
    const i32 dy = static_cast<i32>(ay) - static_cast<i32>(by);
    const u16 adx = static_cast<u16>(dx < 0 ? -dx : dx);
    const u16 ady = static_cast<u16>(dy < 0 ? -dy : dy);
    return adx > ady ? adx : ady;
}

static bool far_enough (u16 x, u16 y, u16 min_d) {
    for (u16 i = 0; i < g_city_n; ++i) {
        if (cheb_dist(x, y, g_cx[i], g_cy[i]) < min_d) {
            return false;
        }
    }
    return true;
}

static u16 collect_city_pts (const GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    g_city_n = 0;
    for (u16 y = G_EDGE_PAD; y + G_EDGE_PAD < h && g_city_n < G_CITY_MAX; ++y) {
        for (u16 x = G_EDGE_PAD; x + G_EDGE_PAD < w && g_city_n < G_CITY_MAX; ++x) {
            if (map.get_river(x, y) == 0) {
                continue;
            }
            if (!is_city_terr(map.get_terrain(x, y)) || !is_city_clim(map.get_climate(x, y))) {
                continue;
            }
            const u16 min_d = clim_min_dist(map.get_climate(x, y));
            if (!far_enough(x, y, min_d)) {
                continue;
            }
            g_cx[g_city_n] = x;
            g_cy[g_city_n] = y;
            g_city_n = static_cast<u16>(g_city_n + 1u);
        }
    }
    return g_city_n;
}

static u16 find_worker_typ (const RuntimeStatics& st) {
    u16 worker_type = U16_KEY_NULL;
    const u16 tn = st.unit_type().get_item_count();
    for (u16 i = 0; i < tn; ++i) {
        cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0) {
            worker_type = i;
            break;
        }
    }
    if (worker_type == U16_KEY_NULL) {
        return U16_KEY_NULL;
    }
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        if (st.unit().get_item(UnitStaticDataKey::from_raw(i)).type == worker_type) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static void refill_mp (GameState& s, u16 unit_idx) {
    UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || s.m_statics == nullptr) {
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    if (u->m_unit_typ_idx >= typ_n) {
        return;
    }
    const u16 pts = s.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * PATH_MP_TURN);
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
    }
}

static void put_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
    p[0] = r;
    p[1] = g;
    p[2] = b;
}

static void draw_line (u8* rgb, u16 w, u16 h, u16 x0, u16 y0, u16 x1, u16 y1) {
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
        put_px(rgb, w, h, static_cast<u16>(ax), static_cast<u16>(ay), 0, 0, 0);
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

static bool wr_net_ppm (const GameState& state, cstr path) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const u16 city_n = state.m_cities.get_city_count();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            terr_rgb(state.m_map.get_terrain(x, y), &p[0], &p[1], &p[2]);
            if (state.m_map.get_river(x, y) != 0) {
                p[0] = 0;
                p[1] = 180;
                p[2] = 255;
            }
        }
    }
    for (u16 i = 0; i < city_n; ++i) {
        const City* a = state.m_cities.get_city(i);
        if (a == nullptr) {
            continue;
        }
        for (u8 d = 0; d < 4u; ++d) {
            const u16 j = a->get_conn_city(d);
            if (j == U16_KEY_NULL || j < i) {
                continue;
            }
            const City* b = state.m_cities.get_city(j);
            if (b == nullptr) {
                continue;
            }
            draw_line(rgb.data(), w, h, a->get_x(), a->get_y(), b->get_x(), b->get_y());
        }
    }
    for (u16 i = 0; i < city_n; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        put_px(rgb.data(), w, h, c->get_x(), c->get_y(), 255, 0, 0);
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool wr_result_ppm (const GameState& state, cstr path) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            terr_rgb(state.m_map.get_terrain(x, y), &p[0], &p[1], &p[2]);
            if (state.m_map.get_river(x, y) != 0) {
                p[0] = 0;
                p[1] = 180;
                p[2] = 255;
            }
            if (state.m_map.tile(x, y)->m_road_typ != ROAD_NONE) {
                p[0] = 92;
                p[1] = 52;
                p[2] = 18;
            }
        }
    }
    for (u16 i = 0; i < g_wk_n; ++i) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(g_wk[i]));
        if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_x >= w || u->m_y >= h) {
            continue;
        }
        u8* p = &rgb[(static_cast<size_t>(u->m_y) * w + u->m_x) * 3u];
        p[0] = 128;
        p[1] = 0;
        p[2] = 0;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.get_add_typ(x, y) != BUILD_ADD_CITY) {
                continue;
            }
            u8* p = &rgb[(static_cast<size_t>(y) * w + x) * 3u];
            p[0] = 0;
            p[1] = 0;
            p[2] = 0;
        }
    }
    FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    const size_t nbytes = static_cast<size_t>(w) * static_cast<size_t>(h) * 3u;
    const bool ok = std::fwrite(rgb.data(), 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    return ok;
}

static bool init_state (GameState* state, RuntimeStatics& st) {
    state->clear();
    state->m_statics = &st;
    if (!UnitMovementMng::setup_mvt_costs(st)) {
        return false;
    }
    state->m_civ_relations.reset(st.civ().get_item_count());
    if (!Factory_GameArraySimple::load_map_gen_data(&state->m_map, g_terr, g_clim, g_riv)) {
        return false;
    }
    if (!Factory_GameArraySimple::load_res_dist_data(&state->m_map, g_res)) {
        return false;
    }
    const u16 w = state->m_map.width();
    const u16 h = state->m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    if (!state->m_cities.bind_statics(st)) {
        return false;
    }
    CityBorder::bind_map(&state->m_map);
    CityTileManager::bind_cities(&state->m_cities);
    PlayerState* seat = new PlayerState();
    if (seat == nullptr) {
        return false;
    }
    seat->m_ai_controlled = 1;
    seat->m_is_active = 1;
    seat->m_civ_index = 0;
    seat->m_explored_overlay = new MapBitOverlay(w, h);
    seat->m_techs_researched = nullptr;
    if (seat->m_explored_overlay == nullptr) {
        delete seat;
        return false;
    }
    state->m_player_states = seat;
    state->m_player_n = 1;
    state->m_players_remaining = 1;
    state->m_land_worker_type_idx = U16_KEY_NULL;
    const u16 tn = st.unit_type().get_item_count();
    for (u16 i = 0; i < tn; ++i) {
        cstr nm = st.unit_type().get_name(UnitTypeStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, "LAND_WORKER") == 0) {
            state->m_land_worker_type_idx = i;
            break;
        }
    }
    return true;
}

static u32 count_roads (const GameState& state) {
    u32 road_n = 0;
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.tile(x, y)->m_road_typ != ROAD_NONE) {
                road_n = road_n + 1u;
            }
        }
    }
    return road_n;
}

//================================================================================================================================
//=> - main -
//================================================================================================================================

int main () {
    if (!build_paths()) {
        std::printf("fail build paths\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("fail mkdir %s\n", G_OUT_DIR);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("fail load runtime statics\n");
        return 1;
    }
    RuntimeStatics& st = loader.statics();
    GameState state;
    if (!init_state(&state, st)) {
        std::printf("fail init state\n");
        return 1;
    }
    const u16 city_n = collect_city_pts(state.m_map);
    if (city_n == 0) {
        std::printf("fail city pts got 0\n");
        return 1;
    }
    std::printf("city_count=%u (cap=%u) river+clim_spacing black=%u grass=%u plains=%u\n",
        (unsigned)city_n, (unsigned)G_CITY_MAX,
        (unsigned)G_DIST_BLACK, (unsigned)G_DIST_GRASS, (unsigned)G_DIST_PLAINS);
    for (u16 i = 0; i < city_n; ++i) {
        const u16 idx = state.m_cities.get_next_new_city_idx();
        City* city = state.m_cities.get_city(idx);
        if (city == nullptr) {
            std::printf("fail city alloc at %u\n", (unsigned)i);
            return 1;
        }
        city->init(0, g_cx[i], g_cy[i]);
        if (!state.m_map.set_tile_add(g_cx[i], g_cy[i], idx, BUILD_ADD_CITY)) {
            std::printf("fail city tile at (%u,%u)\n", (unsigned)g_cx[i], (unsigned)g_cy[i]);
            return 1;
        }
        CityBorder::claim_expand(g_cx[i], g_cy[i], 0, G_CLAIM_CULT, 0);
    }
    const CircArea work = CityTileManager::work_area();
    std::printf("work_reach lim=%u claim_cult=%u\n", (unsigned)work.m_lim, (unsigned)G_CLAIM_CULT);
    const u16 worker_typ = find_worker_typ(st);
    if (worker_typ == U16_KEY_NULL) {
        std::printf("fail find worker typ\n");
        return 1;
    }
    g_wk_n = 0;
    const u16 wk_cap = city_n < G_WORKER_MAX ? city_n : G_WORKER_MAX;
    for (u16 i = 0; i < city_n && g_wk_n < wk_cap; ++i) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, g_cx[i], g_cy[i], 0, worker_typ, &key)) {
            continue;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            continue;
        }
        WorkerHelper::set_data(u, i);
        g_wk[g_wk_n] = key.value();
        g_wk_n = static_cast<u16>(g_wk_n + 1u);
    }
    if (g_wk_n == 0) {
        std::printf("fail spawn workers\n");
        return 1;
    }
    std::printf("workers=%u worker_typ=%u turns=%u img_every=%u\n",
        (unsigned)g_wk_n, (unsigned)worker_typ, (unsigned)G_TURNS, (unsigned)G_IMG_EVERY);
    if (!CityConnector::begin(state)) {
        std::printf("fail city connector begin\n");
        return 1;
    }
    char path[512];
    std::snprintf(path, sizeof(path), "%s/00_network.ppm", G_OUT_DIR);
    if (!wr_net_ppm(state, path)) {
        std::printf("fail write %s\n", path);
        CityConnector::clear();
        state.clear();
        return 1;
    }
    std::printf("wrote %s\n", path);
    u64 handle_ns = 0;
    u64 handle_n = 0;
    for (u32 t = 1; t <= G_TURNS; ++t) {
        for (u16 i = 0; i < g_wk_n; ++i) {
            refill_mp(state, g_wk[i]);
            const auto t0 = std::chrono::high_resolution_clock::now();
            CityConnector::handle(state, g_wk[i]);
            const auto t1 = std::chrono::high_resolution_clock::now();
            handle_ns += static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
            handle_n = handle_n + 1ull;
        }
        if ((t % G_IMG_EVERY) == 0u) {
            std::snprintf(path, sizeof(path), "%s/roads_cities_t%03u.ppm", G_OUT_DIR, (unsigned)t);
            if (!wr_result_ppm(state, path)) {
                std::printf("fail write %s\n", path);
                CityConnector::clear();
                state.clear();
                return 1;
            }
            std::printf("t=%u roads=%u wrote %s\n", (unsigned)t, (unsigned)count_roads(state), path);
        }
    }
    CityConnector::clear();
    CityTileManager::bind_cities(nullptr);
    CityBorder::bind_map(nullptr);
    const double avg_ns = (handle_n == 0) ? 0.0 : static_cast<double>(handle_ns) / static_cast<double>(handle_n);
    const double total_ms = static_cast<double>(handle_ns) / 1000000.0;
    std::printf("handle calls=%llu total_ms=%.2f avg_ns=%.2f\n",
        (unsigned long long)handle_n,
        total_ms,
        avg_ns);
    std::printf("done city_count=%u workers=%u roads=%u\n",
        (unsigned)city_n, (unsigned)g_wk_n, (unsigned)count_roads(state));
    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
