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
#include "target_ordering_flood.h"
#include "tile_transfer.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/target-ordering-flood";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/target-ordering-flood/ownership.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/target-ordering-flood/cities.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/target-ordering-flood/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100u;
static const u32 G_TURN_CAP = 300u;
static const u16 G_TGT_CAP = 256u;

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

static u32 min_city_dist (const GameState& state, u16 a, u16 b) {
    const u16 cn = state.m_cities.get_city_count();
    u32 best = 0xFFFFFFFFu;
    for (u16 i = 0; i < cn; ++i) {
        const City* ca = state.m_cities.get_city(i);
        if (ca == nullptr || ca->get_owner() != a) {
            continue;
        }
        const u16 ax = ca->get_x();
        const u16 ay = ca->get_y();
        for (u16 j = 0; j < cn; ++j) {
            const City* cb = state.m_cities.get_city(j);
            if (cb == nullptr || cb->get_owner() != b) {
                continue;
            }
            const u32 adx = ax > cb->get_x() ? static_cast<u32>(ax - cb->get_x()) : static_cast<u32>(cb->get_x() - ax);
            const u32 ady = ay > cb->get_y() ? static_cast<u32>(ay - cb->get_y()) : static_cast<u32>(cb->get_y() - ay);
            const u32 d = adx + ady;
            if (d < best) {
                best = d;
            }
        }
    }
    return best;
}

static bool pick_seats (const GameState& state, u16* pa, u16* pb) {
    u16 best_a = U16_KEY_NULL;
    u16 best_b = U16_KEY_NULL;
    u32 best_d = 0xFFFFFFFFu;
    u32 best_n = 0;
    for (u16 a = 0; a < state.m_player_n; ++a) {
        const u16 na = count_owner_cities(state, a);
        if (na == 0) {
            continue;
        }
        for (u16 b = static_cast<u16>(a + 1u); b < state.m_player_n; ++b) {
            const u16 nb = count_owner_cities(state, b);
            if (nb == 0) {
                continue;
            }
            const u32 d = min_city_dist(state, a, b);
            if (d == 0xFFFFFFFFu) {
                continue;
            }
            const u32 nsum = static_cast<u32>(na) + static_cast<u32>(nb);
            if (d < best_d || (d == best_d && nsum > best_n)) {
                best_d = d;
                best_n = nsum;
                if (na >= nb) {
                    best_a = a;
                    best_b = b;
                } else {
                    best_a = b;
                    best_b = a;
                }
            }
        }
    }
    if (best_a == U16_KEY_NULL || best_b == U16_KEY_NULL) {
        return false;
    }
    *pa = best_a;
    *pb = best_b;
    return true;
}

static bool pick_staging (const GameState& s, u16 seat, u16 enemy, u16* ox, u16* oy) {
    const u16 cn = s.m_cities.get_city_count();
    u32 best_sc = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        const u16 sx = c->get_x();
        const u16 sy = c->get_y();
        u32 min_d = 0xFFFFFFFFu;
        u64 esx = 0;
        u64 esy = 0;
        u32 en = 0;
        for (u16 j = 0; j < cn; ++j) {
            const City* e = s.m_cities.get_city(j);
            if (e == nullptr || e->get_owner() != enemy) {
                continue;
            }
            const u16 ex = e->get_x();
            const u16 ey = e->get_y();
            esx += ex;
            esy += ey;
            en++;
            const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
            const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
            const u32 d = adx + ady;
            if (d < min_d) {
                min_d = d;
            }
        }
        if (en == 0u || min_d == 0xFFFFFFFFu) {
            continue;
        }
        const u16 ecx = static_cast<u16>(esx / en);
        const u16 ecy = static_cast<u16>(esy / en);
        const u32 cdx = sx > ecx ? static_cast<u32>(sx - ecx) : static_cast<u32>(ecx - sx);
        const u32 cdy = sy > ecy ? static_cast<u32>(sy - ecy) : static_cast<u32>(ecy - sy);
        const u32 sc = min_d * 64u + cdx + cdy;
        if (!found || sc < best_sc) {
            best_sc = sc;
            bx = sx;
            by = sy;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

static bool walk_ok (const GameState& st, u16 x, u16 y) {
    const u8 t = st.m_map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
        return false;
    }
    return true;
}

static bool find_seed (const GameState& st, u16 sx, u16 sy, u8 enemy, u16* ox, u16* oy) {
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    u32 best = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (st.m_map.get_civ_owner(x, y) != enemy || !walk_ok(st, x, y)) {
                continue;
            }
            const u32 adx = sx > x ? static_cast<u32>(sx - x) : static_cast<u32>(x - sx);
            const u32 ady = sy > y ? static_cast<u32>(sy - y) : static_cast<u32>(y - sy);
            const u32 d = adx + ady;
            if (!found || d < best) {
                best = d;
                bx = x;
                by = y;
                found = true;
            }
        }
    }
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
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

static const u8* seat_rgb (u8 seat, u8 attacker, u8 defender) {
    if (seat == attacker) {
        return k_own_pal[0];
    }
    if (seat == defender) {
        return k_own_pal[1];
    }
    return k_own_pal[seat % k_own_pal_n];
}

static void blend_own (u8* r, u8* g, u8* b, u8 seat, u8 attacker, u8 defender) {
    if (seat == U8_KEY_NULL) {
        return;
    }
    const u8* c = seat_rgb(seat, attacker, defender);
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

static bool write_owner_ppm (cstr path, const GameState& state, const std::vector<u8>& cap, u8 attacker, u8 defender) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::vector<u8> rgb(static_cast<size_t>(w) * static_cast<size_t>(h) * 3u);
    const u8* atk = k_own_pal[0];
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
            blend_own(&r, &g, &b, state.m_map.get_civ_owner(x, y), attacker, defender);
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
            paint_dot(rgb, w, h, c->get_x(), c->get_y(), atk[0], atk[1], atk[2]);
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

    u16 pa = 0;
    u16 pb = 0;
    if (!pick_seats(state, &pa, &pb)) {
        std::printf("FAIL: need two seats with cities\n");
        state.clear();
        return 1;
    }
    u16 sx = 0;
    u16 sy = 0;
    if (!pick_staging(state, pa, pb, &sx, &sy)) {
        std::printf("FAIL: pick_staging\n");
        state.clear();
        return 1;
    }
    std::printf("attacker=%u defender=%u staging=(%u,%u) cities_A=%u cities_B=%u\n",
        static_cast<unsigned>(pa), static_cast<unsigned>(pb),
        static_cast<unsigned>(sx), static_cast<unsigned>(sy),
        static_cast<unsigned>(count_owner_cities(state, pa)),
        static_cast<unsigned>(count_owner_cities(state, pb)));

    u16 seed_x = 0;
    u16 seed_y = 0;
    if (!find_seed(state, sx, sy, static_cast<u8>(pb), &seed_x, &seed_y)) {
        std::printf("FAIL: no enemy seed tile\n");
        state.clear();
        return 1;
    }
    std::printf("flood seed=(%u,%u)\n", static_cast<unsigned>(seed_x), static_cast<unsigned>(seed_y));

    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    u16 tgts[G_TGT_CAP];
    TargetOrderingFlood flood;
    flood.set_enemy(static_cast<u8>(pb));
    const auto t0 = std::chrono::steady_clock::now();
    const u16 tn = flood.fill(state, seed_x, seed_y, tgts, G_TGT_CAP);
    const auto t1 = std::chrono::steady_clock::now();
    const f64 flood_us = std::chrono::duration<f64, std::micro>(t1 - t0).count();
    std::printf("flood targets=%u flood_us=%.2f cap=%u\n",
        static_cast<unsigned>(tn), flood_us, static_cast<unsigned>(G_TGT_CAP));
    if (tn == 0) {
        std::printf("FAIL: empty target list\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }

    std::vector<u8> cap(state.m_cities.get_city_count(), 0u);
    char base_ppm[400];
    std::snprintf(base_ppm, sizeof(base_ppm), "%s/target_0000.ppm", G_OUT_DIR);
    write_owner_ppm(base_ppm, state, cap, static_cast<u8>(pa), static_cast<u8>(pb));

    for (u16 i = 0; i < tn; ++i) {
        const u16 cidx = tgts[i];
        City* c = state.m_cities.get_city(cidx);
        if (c == nullptr || c->get_owner() != pb) {
            continue;
        }
        const u16 cx = c->get_x();
        const u16 cy = c->get_y();
        const u8 from = static_cast<u8>(pb);
        c->set_owner(pa);
        const u32 flipped = TileTransfer::apply(state, cidx, from, static_cast<u8>(pa), nullptr);
        if (cidx >= cap.size()) {
            cap.resize(static_cast<size_t>(cidx) + 1u, 0u);
        }
        cap[cidx] = 1u;
        char ppm[400];
        std::snprintf(ppm, sizeof(ppm), "%s/target_%04u.ppm", G_OUT_DIR, static_cast<unsigned>(i + 1u));
        write_owner_ppm(ppm, state, cap, static_cast<u8>(pa), static_cast<u8>(pb));
        std::printf("target %u city=(%u,%u) idx=%u tiles=%u\n",
            static_cast<unsigned>(i + 1u),
            static_cast<unsigned>(cx), static_cast<unsigned>(cy),
            static_cast<unsigned>(cidx),
            static_cast<unsigned>(flipped));
    }

    std::printf("done targets=%u flood_us=%.2f out=%s\n",
        static_cast<unsigned>(tn), flood_us, G_OUT_DIR);
    WhiteboardMng::terminate();
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
