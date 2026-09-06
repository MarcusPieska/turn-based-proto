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

#include "game_state.h"
#include "build_adds_array.h"
#include "circular_tile_areas.h"
#include "city.h"
#include "city_border.h"
#include "game_array_simple.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "tile_transfer.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/tile-transfer-test";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/tile-transfer-test/ownership.bin";
static const char* G_CULT = "/home/w/Projects/simple-map-gen/tile-transfer-test/cities_culture.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/tile-transfer-test/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 20u;
static const u32 G_TURN_CAP = 300u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

static const u8 k_own_pal[][3] = {
    {220, 40, 40}, {40, 90, 220}, {40, 170, 70}, {220, 110, 30},
    {190, 40, 170}, {30, 170, 170}, {150, 70, 30}, {100, 40, 180},
    {240, 200, 40}, {40, 200, 220}, {180, 80, 80}, {80, 80, 200},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));
static const char* k_col_ok = "\033[32m";
static const char* k_col_bad = "\033[91m";
static const char* k_col_off = "\033[0m";

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

static bool file_exists (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static u32 rng_next (u32* s) {
    *s = (*s) * 1664525u + 1013904223u;
    return *s;
}

static bool save_ownership (const GameState& state) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::FILE* fp = std::fopen(G_OWN, "wb");
    if (fp == nullptr) {
        return false;
    }
    if (std::fwrite(&w, sizeof(w), 1, fp) != 1 || std::fwrite(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 o = state.m_map.get_civ_owner(x, y);
            if (std::fwrite(&o, sizeof(o), 1, fp) != 1) {
                std::fclose(fp);
                return false;
            }
        }
    }
    std::fclose(fp);
    return true;
}

static bool load_ownership (GameState& state) {
    std::FILE* fp = std::fopen(G_OWN, "rb");
    if (fp == nullptr) {
        return false;
    }
    u16 w = 0;
    u16 h = 0;
    if (std::fread(&w, sizeof(w), 1, fp) != 1 || std::fread(&h, sizeof(h), 1, fp) != 1) {
        std::fclose(fp);
        return false;
    }
    if (w != state.m_map.width() || h != state.m_map.height()) {
        std::fclose(fp);
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 o = U8_KEY_NULL;
            if (std::fread(&o, sizeof(o), 1, fp) != 1) {
                std::fclose(fp);
                return false;
            }
            state.m_map.set_civ_owner(x, y, o);
        }
    }
    std::fclose(fp);
    return true;
}

static bool save_culture (const GameState& state) {
    std::FILE* fp = std::fopen(G_CULT, "w");
    if (fp == nullptr) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        std::fprintf(fp, "%u:%u:%u\n",
            static_cast<unsigned>(c->get_x()),
            static_cast<unsigned>(c->get_y()),
            static_cast<unsigned>(c->get_current_culture()));
    }
    std::fclose(fp);
    return true;
}

static bool found_city (GameState& state, u16 x, u16 y, u16 player) {
    if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        return true;
    }
    const u16 city_idx = state.m_cities.get_next_new_city_idx();
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr) {
        return false;
    }
    city->init(player, x, y);
    if (!state.m_map.set_tile_add(x, y, city_idx, BUILD_ADD_CITY)) {
        return false;
    }
    (void)state.city_net_on_found(city_idx);
    return true;
}

static City* city_at (GameState& state, u16 x, u16 y) {
    if (state.m_map.get_add_typ(x, y) != BUILD_ADD_CITY) {
        return nullptr;
    }
    return state.m_cities.get_city(state.m_map.get_add_idx(x, y));
}

static bool load_culture (GameState& state) {
    std::FILE* fp = std::fopen(G_CULT, "r");
    if (fp == nullptr) {
        return false;
    }
    unsigned x = 0;
    unsigned y = 0;
    unsigned cult = 0;
    u32 n = 0;
    while (std::fscanf(fp, "%u:%u:%u\n", &x, &y, &cult) == 3) {
        if (x >= state.m_map.width() || y >= state.m_map.height()) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        const u8 own = state.m_map.get_civ_owner(ux, uy);
        if (own == U8_KEY_NULL || own >= state.m_player_n) {
            continue;
        }
        if (!found_city(state, ux, uy, own)) {
            std::fclose(fp);
            return false;
        }
        City* c = city_at(state, ux, uy);
        if (c == nullptr) {
            std::fclose(fp);
            return false;
        }
        c->set_owner(own);
        c->set_culture(static_cast<u16>(cult > 65535u ? 65535u : cult));
        ++n;
    }
    std::fclose(fp);
    std::printf("loaded culture rows=%u\n", static_cast<unsigned>(n));
    return n > 0;
}

static void terr_rgb (u8 terr, u8* r, u8* g, u8* b) {
    static const u8* k_terr[] = {
        TERR_NONE, TERR_OCEAN, TERR_SEA, TERR_COASTAL, TERR_PLAINS, TERR_HILLS, TERR_MOUNTAINS, TERR_VOLCANO,
        TERR_INLAND_SEA, TERR_INLAND_LAKE
    };
    for (u16 i = 0; i < 10u; ++i) {
        if (k_terr[i][0] == terr) {
            *r = k_terr[i][1];
            *g = k_terr[i][2];
            *b = k_terr[i][3];
            return;
        }
    }
    *r = 40;
    *g = 40;
    *b = 40;
}

static void blend_own (u8* r, u8* g, u8* b, u8 seat) {
    if (seat == U8_KEY_NULL) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    *r = static_cast<u8>((static_cast<u16>(*r) + static_cast<u16>(c[0]) * 3u) / 4u);
    *g = static_cast<u8>((static_cast<u16>(*g) + static_cast<u16>(c[1]) * 3u) / 4u);
    *b = static_cast<u8>((static_cast<u16>(*b) + static_cast<u16>(c[2]) * 3u) / 4u);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, bool white) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + x) * 3u;
    const u8 v = white ? 255u : 0u;
    rgb[i] = v;
    rgb[i + 1] = v;
    rgb[i + 2] = v;
}

static bool write_owner_ppm (cstr path, const GameState& state, const std::vector<u8>& cap) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(state.m_map.get_terrain(x, y), &r, &g, &b);
            if (state.m_map.get_river(x, y) != 0u) {
                r = 40;
                g = 90;
                b = 200;
            }
            blend_own(&r, &g, &b, state.m_map.get_civ_owner(x, y));
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        const bool white = i < cap.size() && cap[i] != 0u;
        paint_dot(rgb, w, h, c->get_x(), c->get_y(), white);
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const bool ok = std::fwrite(rgb.data(), 1, rgb.size(), fp) == rgb.size();
    std::fclose(fp);
    return ok;
}

static bool run_warmup (GameState& state) {
    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        return false;
    }
    while (state.m_current_turn < G_TURN_CAP) {
        if (!loop.step()) {
            break;
        }
        std::printf("\rwarmup turn %u / %u", state.m_current_turn, G_TURN_CAP);
        std::fflush(stdout);
    }
    std::printf("\n");
    loop.end();
    return save_ownership(state) && save_culture(state);
}

static u16 count_owner_cities (const GameState& state, u16 owner) {
    u16 n = 0;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_owner() == owner) {
            ++n;
        }
    }
    return n;
}

static bool pick_city (GameState& state, u16 owner, u32* rng, u16* out_idx) {
    const u16 n = count_owner_cities(state, owner);
    if (n == 0) {
        return false;
    }
    u16 pick = static_cast<u16>(rng_next(rng) % n);
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != owner) {
            continue;
        }
        if (pick == 0) {
            *out_idx = i;
            return true;
        }
        --pick;
    }
    return false;
}

static bool pick_seats (const GameState& state, u16* pa, u16* pb) {
    u16 best_a = U16_KEY_NULL;
    u16 best_b = U16_KEY_NULL;
    u16 best_an = 0;
    u16 best_bn = 0;
    for (u16 p = 0; p < state.m_player_n; ++p) {
        const u16 n = count_owner_cities(state, p);
        if (n > best_an) {
            best_bn = best_an;
            best_b = best_a;
            best_an = n;
            best_a = p;
        } else if (n > best_bn) {
            best_bn = n;
            best_b = p;
        }
    }
    if (best_a == U16_KEY_NULL || best_b == U16_KEY_NULL || best_bn == 0) {
        return false;
    }
    *pa = best_a;
    *pb = best_b;
    return true;
}

static bool city_encased (const GameState& state, u16 cx, u16 cy, u8 own) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    const CircArea disc = CircularTileAreas::get(2);
    const u16 i0 = CircularTileAreas::get(1).m_lim;
    u16 n = 0;
    for (u16 i = i0; i < disc.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(disc.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(disc.m_brd[i][1]);
        if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
            continue;
        }
        ++n;
        if (state.m_map.get_civ_owner(static_cast<u16>(x), static_cast<u16>(y)) == own) {
            return false;
        }
    }
    return n > 0;
}

static bool chk_encase (const GameState& state, u16 pa, u16 pb, u16 step) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 own = c->get_owner();
        if (own != pa && own != pb) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        if (!city_encased(state, cx, cy, static_cast<u8>(own))) {
            continue;
        }
        std::printf("%sFAIL: step %u city=(%u,%u) owner=%u fully encased by non-self tiles%s\n",
            k_col_bad,
            static_cast<unsigned>(step),
            static_cast<unsigned>(cx), static_cast<unsigned>(cy),
            static_cast<unsigned>(own),
            k_col_off);
        return false;
    }
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("path/out failed\n");
        return 1;
    }

    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    CityBorder::bind_map(&state.m_map);

    const bool have_cache = file_exists(G_OWN) && file_exists(G_CULT);
    if (have_cache) {
        std::printf("cache hit: loading ownership + culture\n");
        if (!load_ownership(state) || !load_culture(state)) {
            std::printf("FAIL: cache load\n");
            state.clear();
            return 1;
        }
    } else {
        std::printf("cache miss: running %u turns\n", G_TURN_CAP);
        if (!run_warmup(state)) {
            std::printf("FAIL: warmup/save\n");
            state.clear();
            return 1;
        }
        std::printf("saved %s and %s\n", G_OWN, G_CULT);
    }

    u16 pa = 0;
    u16 pb = 0;
    if (!pick_seats(state, &pa, &pb)) {
        std::printf("FAIL: need two seats with cities\n");
        state.clear();
        return 1;
    }
    std::printf("transfer B=%u (%u cities) -> A=%u (%u cities)\n",
        static_cast<unsigned>(pb), static_cast<unsigned>(count_owner_cities(state, pb)),
        static_cast<unsigned>(pa), static_cast<unsigned>(count_owner_cities(state, pa)));

    char base_ppm[400];
    std::snprintf(base_ppm, sizeof(base_ppm), "%s/transfer_0000.ppm", G_OUT_DIR);
    std::vector<u8> cap(state.m_cities.get_city_count(), 0u);
    write_owner_ppm(base_ppm, state, cap);

    u32 rng = G_SEED;
    u16 step = 0;
    f64 tot_us = 0.0;
    u32 tot_tiles = 0;
    while (count_owner_cities(state, pb) > 0) {
        u16 cidx = U16_KEY_NULL;
        if (!pick_city(state, pb, &rng, &cidx)) {
            break;
        }
        City* c = state.m_cities.get_city(cidx);
        if (c == nullptr) {
            break;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        const u16 cult = c->get_current_culture();
        const u8 from = static_cast<u8>(pb);
        c->set_owner(pa);
        u16 near_n = 0;
        const auto t0 = std::chrono::steady_clock::now();
        const u32 n = TileTransfer::apply(state, cidx, from, static_cast<u8>(pa), &near_n);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
        tot_us += us;
        tot_tiles = static_cast<u32>(tot_tiles + n);
        if (cidx >= cap.size()) {
            cap.resize(static_cast<size_t>(cidx) + 1u, 0u);
        }
        cap[cidx] = 1u;
        ++step;
        char ppm[400];
        std::snprintf(ppm, sizeof(ppm), "%s/transfer_%04u.ppm", G_OUT_DIR, static_cast<unsigned>(step));
        write_owner_ppm(ppm, state, cap);
        const u16 R_eff = CityBorder::radius_for(cult);
        const u16 R_floor = CityBorder::radius_for(25);
        const char* col = n > 0 ? k_col_ok : k_col_bad;
        std::printf("step %u city=(%u,%u) cult=%u R=%u near=%u %stiles_transferred=%u%s apply_us=%.2f remain_B=%u\n",
            static_cast<unsigned>(step),
            static_cast<unsigned>(cx), static_cast<unsigned>(cy),
            static_cast<unsigned>(cult),
            static_cast<unsigned>(R_eff < R_floor ? R_floor : R_eff),
            static_cast<unsigned>(near_n),
            col, static_cast<unsigned>(n), k_col_off, us,
            static_cast<unsigned>(count_owner_cities(state, pb)));
        if (!chk_encase(state, pa, pb, step)) {
            state.clear();
            setup.release_map_gen();
            return 1;
        }
    }

    std::printf("done steps=%u tiles_tot=%u apply_us_avg=%.2f apply_us_tot=%.2f out=%s\n",
        static_cast<unsigned>(step),
        static_cast<unsigned>(tot_tiles),
        step == 0 ? 0.0 : tot_us / static_cast<f64>(step),
        tot_us,
        G_OUT_DIR);
    state.clear();
    setup.release_map_gen();
    return step == 0 ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
