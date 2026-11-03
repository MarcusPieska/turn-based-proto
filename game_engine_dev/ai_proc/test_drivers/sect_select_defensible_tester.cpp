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

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "game_array_simple.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "gen_land_sector_network.h"
#include "gen_land_sectors.h"
#include "land_sector_network.h"
#include "expansion_state.h"
#include "land_sector_adj.h"
#include "sector_support.h"
#include "sect_select_defensible.h"
#include "tile_transfer.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/sect-select-defensible";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/sect-select-defensible/ownership.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/sect-select-defensible/cities.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/sect-select-defensible/game_loop.trace";
static const u32 G_SEED = 43u;
static const u32 G_RNG = 43u;
static const u16 G_PLAYERS = 100u;
static const u32 G_TURN_CAP = 300u;
static const u16 G_TGT_CAP = 256u;
static const u16 G_STEP_CAP = 500u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];
static char g_flags[320];

static const u8 k_own_pal[][3] = {
    {220, 40, 40}, {40, 90, 220}, {40, 170, 70}, {220, 110, 30},
    {190, 40, 170}, {30, 170, 170}, {150, 70, 30}, {100, 40, 180},
    {240, 200, 40}, {40, 200, 220}, {180, 80, 80}, {80, 80, 200},
    {200, 140, 40}, {60, 120, 60}, {160, 50, 90}, {50, 140, 200},
    {230, 80, 140}, {90, 160, 40}, {130, 90, 220}, {210, 160, 80},
    {40, 130, 130}, {180, 120, 180}, {100, 100, 40}, {70, 70, 160},
    {240, 100, 60}, {50, 180, 120}, {170, 60, 40}, {120, 180, 220},
    {200, 60, 100}, {80, 200, 80}, {140, 40, 140}, {40, 100, 180},
    {230, 180, 100}, {100, 140, 100}, {180, 100, 140}, {60, 160, 200},
    {200, 200, 60}, {140, 80, 60}, {80, 60, 120}, {160, 200, 160},
    {220, 60, 180}, {60, 200, 160}, {180, 160, 40}, {100, 60, 200},
    {240, 140, 120}, {40, 160, 80}, {160, 40, 60}, {120, 120, 220},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));

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
    if (std::snprintf(g_flags, sizeof(g_flags), "%s/flags.ppm", dir) <= 0) {
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

static bool save_cities (const GameState& state) {
    std::FILE* fp = std::fopen(G_CITIES, "w");
    if (fp == nullptr) {
        return false;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        std::fprintf(fp, "%u:%u:%u:%u\n",
            static_cast<unsigned>(c->get_x()),
            static_cast<unsigned>(c->get_y()),
            static_cast<unsigned>(c->get_owner()),
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

static bool load_cities (GameState& state) {
    std::FILE* fp = std::fopen(G_CITIES, "r");
    if (fp == nullptr) {
        return false;
    }
    unsigned x = 0;
    unsigned y = 0;
    unsigned own = 0;
    unsigned cult = 0;
    u32 n = 0;
    while (std::fscanf(fp, "%u:%u:%u:%u\n", &x, &y, &own, &cult) == 4) {
        if (x >= state.m_map.width() || y >= state.m_map.height() || own >= state.m_player_n) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (!found_city(state, ux, uy, static_cast<u16>(own))) {
            std::fclose(fp);
            return false;
        }
        City* c = city_at(state, ux, uy);
        if (c == nullptr) {
            std::fclose(fp);
            return false;
        }
        c->set_owner(static_cast<u16>(own));
        c->set_culture(static_cast<u16>(cult > 65535u ? 65535u : cult));
        ++n;
    }
    std::fclose(fp);
    std::printf("loaded cities=%u\n", static_cast<unsigned>(n));
    return n > 0;
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
    return save_ownership(state) && save_cities(state);
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

static bool pick_expand_seat (const GameState& state, u16* out) {
    u16 best = U16_KEY_NULL;
    u16 best_n = 0u;
    for (u16 p = 0; p < state.m_player_n; ++p) {
        const u16 n = count_owner_cities(state, p);
        if (n > best_n) {
            best_n = n;
            best = p;
        }
    }
    if (best == U16_KEY_NULL || best_n == 0u) {
        return false;
    }
    *out = best;
    return true;
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

static const u8* seat_rgb (u8 seat) {
    return k_own_pal[seat % k_own_pal_n];
}

static void blend_own (u8* r, u8* g, u8* b, u8 seat) {
    if (seat == U8_KEY_NULL) {
        return;
    }
    const u8* c = seat_rgb(seat);
    *r = static_cast<u8>((static_cast<u16>(*r) + static_cast<u16>(c[0]) * 3u) / 4u);
    *g = static_cast<u8>((static_cast<u16>(*g) + static_cast<u16>(c[1]) * 3u) / 4u);
    *b = static_cast<u8>((static_cast<u16>(*b) + static_cast<u16>(c[2]) * 3u) / 4u);
}

static void paint_dot (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + x) * 3u;
    rgb[i] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void order_to_rgb (u16 step, u16 n, u8* r, u8* g, u8* b) {
    float t = 0.0f;
    if (n > 1u) {
        t = static_cast<float>(step) / static_cast<float>(n - 1u);
    }
    if (t < 0.0f) {
        t = 0.0f;
    }
    if (t > 1.0f) {
        t = 1.0f;
    }
    *r = static_cast<u8>((1.0f - t) * 220.0f + 20.0f);
    *g = static_cast<u8>(40.0f);
    *b = static_cast<u8>(t * 220.0f + 20.0f);
}

static void put_px (std::vector<u8>& rgb, u16 w, u16 h, i32 x, i32 y, u8 r, u8 g, u8 b) {
    if (x < 0 || y < 0 || x >= static_cast<i32>(w) || y >= static_cast<i32>(h)) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * w + static_cast<u32>(x)) * 3u;
    rgb[i] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void put_sq3 (std::vector<u8>& rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    for (i32 dy = -1; dy <= 1; ++dy) {
        for (i32 dx = -1; dx <= 1; ++dx) {
            put_px(rgb, w, h, static_cast<i32>(x) + dx, static_cast<i32>(y) + dy, r, g, b);
        }
    }
}

static void draw_line (
    std::vector<u8>& rgb,
    u16 w,
    u16 h,
    u16 x0,
    u16 y0,
    u16 x1,
    u16 y1,
    u8 r,
    u8 g,
    u8 b) {
    i32 dx = static_cast<i32>(x1) - static_cast<i32>(x0);
    i32 dy = static_cast<i32>(y1) - static_cast<i32>(y0);
    const i32 adx = dx < 0 ? -dx : dx;
    const i32 ady = dy < 0 ? -dy : dy;
    const i32 sx = dx < 0 ? -1 : 1;
    const i32 sy = dy < 0 ? -1 : 1;
    i32 err = adx - ady;
    i32 x = static_cast<i32>(x0);
    i32 y = static_cast<i32>(y0);
    for (;;) {
        put_px(rgb, w, h, x, y, r, g, b);
        if (x == static_cast<i32>(x1) && y == static_cast<i32>(y1)) {
            break;
        }
        const i32 e2 = err * 2;
        if (e2 > -ady) {
            err -= ady;
            x += sx;
        }
        if (e2 < adx) {
            err += adx;
            y += sy;
        }
    }
}

static bool write_net_ppm (
    cstr path,
    const GameArraySimple& map,
    const LandSectorSeeds& seeds,
    const LandSectorNetwork& net) {
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
                r = 28u;
                g = 55u;
                b = 110u;
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
    if (seeds.m_pts != nullptr) {
        for (u16 i = 0; i < net.link_n(); ++i) {
            const LandSectorLink* L = net.get(i);
            if (L == nullptr || L->m_a >= seeds.m_n || L->m_b >= seeds.m_n) {
                continue;
            }
            draw_line(rgb, w, h,
                seeds.m_pts[L->m_a].m_x, seeds.m_pts[L->m_a].m_y,
                seeds.m_pts[L->m_b].m_x, seeds.m_pts[L->m_b].m_y,
                255u, 255u, 255u);
        }
        for (u16 i = 0; i < seeds.m_n; ++i) {
            put_sq3(rgb, w, h, seeds.m_pts[i].m_x, seeds.m_pts[i].m_y, 240u, 220u, 40u);
        }
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

static bool write_order_ppm (
    cstr path,
    const GameArraySimple& map,
    const GenLandSectors& gls,
    const LandSectorSeeds& seeds,
    const std::vector<u16>& ord) {
    const u16 w = map.width();
    const u16 h = map.height();
    const u16 sn = gls.sector_n();
    const u16 on = static_cast<u16>(ord.size());
    const Whiteboard_2B& sec = gls.sectors();
    std::vector<u16> rank(sn, U16_KEY_NULL);
    for (u16 i = 0; i < on; ++i) {
        const u16 s = ord[i];
        if (s < sn && rank[s] == U16_KEY_NULL) {
            rank[s] = i;
        }
    }
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
                if (tag != GLS_IDX_NONE && tag <= sn) {
                    const u16 id = static_cast<u16>(tag - 1u);
                    if (rank[id] != U16_KEY_NULL) {
                        order_to_rgb(rank[id], on > 0u ? on : 1u, &r, &g, &b);
                    }
                }
            }
            const u32 i = (static_cast<u32>(y) * w + x) * 3u;
            rgb[i] = r;
            rgb[i + 1] = g;
            rgb[i + 2] = b;
        }
    }
    if (seeds.m_pts != nullptr) {
        for (u16 i = 1; i < on; ++i) {
            const u16 a = ord[static_cast<size_t>(i - 1u)];
            const u16 b = ord[i];
            if (a >= seeds.m_n || b >= seeds.m_n) {
                continue;
            }
            draw_line(rgb, w, h,
                seeds.m_pts[a].m_x, seeds.m_pts[a].m_y,
                seeds.m_pts[b].m_x, seeds.m_pts[b].m_y,
                12u, 12u, 12u);
        }
        for (u16 i = 0; i < on; ++i) {
            const u16 s = ord[i];
            if (s >= seeds.m_n) {
                continue;
            }
            put_sq3(rgb, w, h, seeds.m_pts[s].m_x, seeds.m_pts[s].m_y, 40u, 220u, 40u);
        }
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

static bool write_owner_ppm (cstr path, const GameState& state, const std::vector<u8>& cap, u8 expander) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    const u8* exp = seat_rgb(expander);
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
        const bool taken = i < cap.size() && cap[i] != 0u;
        if (taken) {
            paint_dot(rgb, w, h, c->get_x(), c->get_y(), exp[0], exp[1], exp[2]);
        } else {
            paint_dot(rgb, w, h, c->get_x(), c->get_y(), 0u, 0u, 0u);
        }
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
    paths.m_flags = g_flags;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("setup_new_game failed\n");
        return 1;
    }
    CityBorder::bind_map(&state.m_map);

    const bool have_cache = file_exists(G_OWN) && file_exists(G_CITIES);
    if (have_cache) {
        std::printf("cache hit: loading ownership + cities\n");
        if (!load_ownership(state) || !load_cities(state)) {
            std::printf("FAIL: cache load\n");
            state.clear();
            return 1;
        }
    } else {
        std::printf("cache miss: running %u turns with %u players\n", G_TURN_CAP, G_PLAYERS);
        if (!run_warmup(state)) {
            std::printf("FAIL: warmup/save\n");
            state.clear();
            return 1;
        }
        std::printf("saved %s and %s\n", G_OWN, G_CITIES);
    }

    u16 expander = U16_KEY_NULL;
    if (!pick_expand_seat(state, &expander)) {
        std::printf("FAIL: no expand seat\n");
        state.clear();
        return 1;
    }
    std::printf("expander=%u cities=%u\n",
        static_cast<unsigned>(expander),
        static_cast<unsigned>(count_owner_cities(state, expander)));

    GenLandSectors gls;
    if (!gls.begin(state.m_map)) {
        std::printf("FAIL: GenLandSectors::begin\n");
        state.clear();
        return 1;
    }
    LandSectorSeeds seeds = {};
    if (!gls.gen_seeds(G_RNG, &seeds) || seeds.m_n == 0u) {
        std::printf("FAIL: gen_seeds\n");
        state.clear();
        return 1;
    }
    LandSectorNetwork net;
    if (!GenLandSectorNetwork::build(gls.sectors(), gls.sector_n(), state.m_map, &net) || !net.ok()) {
        std::printf("FAIL: GenLandSectorNetwork::build\n");
        GenLandSectors::free_seeds(&seeds);
        state.clear();
        return 1;
    }
    std::printf("sectors=%u links=%u\n",
        static_cast<unsigned>(seeds.m_n),
        static_cast<unsigned>(net.link_n()));
    char net_ppm[400];
    std::snprintf(net_ppm, sizeof(net_ppm), "%s/sector_network.ppm", G_OUT_DIR);
    if (!write_net_ppm(net_ppm, state.m_map, seeds, net)) {
        std::printf("FAIL: write %s\n", net_ppm);
        GenLandSectors::free_seeds(&seeds);
        state.clear();
        return 1;
    }
    std::printf("wrote %s\n", net_ppm);
    if (!SectorSupport::bind(&state, &gls.sectors(), &seeds, &net)) {
        std::printf("FAIL: SectorSupport::bind\n");
        GenLandSectors::free_seeds(&seeds);
        state.clear();
        return 1;
    }

    LandSectorAdj adj;
    if (!adj.build(net, gls.sector_n())) {
        std::printf("FAIL: LandSectorAdj::build\n");
        SectorSupport::clr();
        GenLandSectors::free_seeds(&seeds);
        state.clear();
        return 1;
    }
    ExpansionState exp;
    if (!SectorSelect_Defensible::begin(exp, adj, seeds.m_pts, seeds.m_n, G_RNG)) {
        std::printf("FAIL: SectorSelect_Defensible::begin\n");
        SectorSupport::clr();
        GenLandSectors::free_seeds(&seeds);
        state.clear();
        return 1;
    }
    std::printf("walk start=%u\n", static_cast<unsigned>(exp.start()));

    std::vector<u8> cap(state.m_cities.get_city_count(), 0u);
    std::vector<u16> ord;
    char base_ppm[400];
    std::snprintf(base_ppm, sizeof(base_ppm), "%s/expand_0000.ppm", G_OUT_DIR);
    write_owner_ppm(base_ppm, state, cap, static_cast<u8>(expander));

    u16 tgts[G_TGT_CAP];
    u16 step = 0u;
    u16 flipped_n = 0u;
    u16 sector = U16_KEY_NULL;
    u32 edge_n = 0u;
    u32 fwd_n = 0u;
    u32 back_n = 0u;
    while (step < G_STEP_CAP) {
        const bool emit0 = exp.have();
        const u16 pn0 = exp.path_n();
        if (!SectorSelect_Defensible::select(exp, adj, seeds.m_pts, &sector)) {
            const u32 pops = static_cast<u32>(pn0 - exp.path_n());
            back_n += pops;
            edge_n += pops;
            break;
        }
        if (!emit0) {
            const u32 pops = static_cast<u32>(pn0 + 1u - exp.path_n());
            back_n += pops;
            fwd_n += 1u;
            edge_n += pops + 1u;
        }
        ord.push_back(sector);
        u16 tn = 0u;
        u16 enemy = U16_KEY_NULL;
        const SectorPresence pr = SectorSupport::sector_presence(expander, sector);
        enemy = pr.m_rival;
        if (enemy == U16_KEY_NULL) {
            continue;
        }
        tn = SectorSupport::enemy_cities(sector, enemy, tgts, G_TGT_CAP);
        if (tn == 0u) {
            continue;
        }
        std::printf("step %u sector=%u enemy=%u cities=%u\n",
            static_cast<unsigned>(step + 1u),
            static_cast<unsigned>(sector),
            static_cast<unsigned>(enemy),
            static_cast<unsigned>(tn));
        for (u16 i = 0; i < tn; ++i) {
            const u16 cidx = tgts[i];
            City* c = state.m_cities.get_city(cidx);
            if (c == nullptr || c->get_owner() != enemy) {
                continue;
            }
            const u16 cx = c->get_x();
            const u16 cy = c->get_y();
            const u8 from = static_cast<u8>(enemy);
            c->set_owner(expander);
            const u32 tiles = TileTransfer::apply(state, cidx, from, static_cast<u8>(expander), nullptr);
            if (cidx >= cap.size()) {
                cap.resize(static_cast<size_t>(cidx) + 1u, 0u);
            }
            cap[cidx] = 1u;
            ++flipped_n;
            std::printf("  city=(%u,%u) idx=%u tiles=%u\n",
                static_cast<unsigned>(cx), static_cast<unsigned>(cy),
                static_cast<unsigned>(cidx),
                static_cast<unsigned>(tiles));
        }
        ++step;
        char ppm[400];
        std::snprintf(ppm, sizeof(ppm), "%s/expand_%04u.ppm", G_OUT_DIR, static_cast<unsigned>(step));
        write_owner_ppm(ppm, state, cap, static_cast<u8>(expander));
    }
    std::printf("selector walk done flips=%u order_n=%u\n",
        static_cast<unsigned>(step),
        static_cast<unsigned>(ord.size()));
    std::printf("edge_traversals=%u fwd=%u back=%u\n",
        static_cast<unsigned>(edge_n),
        static_cast<unsigned>(fwd_n),
        static_cast<unsigned>(back_n));

    char ord_ppm[400];
    std::snprintf(ord_ppm, sizeof(ord_ppm), "%s/select_order.ppm", G_OUT_DIR);
    if (!write_order_ppm(ord_ppm, state.m_map, gls, seeds, ord)) {
        std::printf("FAIL: write %s\n", ord_ppm);
    } else {
        std::printf("wrote %s order_n=%u\n", ord_ppm, static_cast<unsigned>(ord.size()));
    }

    std::printf("done steps=%u flipped_cities=%u expander_cities=%u out=%s\n",
        static_cast<unsigned>(step),
        static_cast<unsigned>(flipped_n),
        static_cast<unsigned>(count_owner_cities(state, expander)),
        G_OUT_DIR);

    SectorSupport::clr();
    GenLandSectors::free_seeds(&seeds);
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
