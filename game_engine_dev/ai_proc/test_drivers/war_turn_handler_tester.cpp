//================================================================================================================================
//=> - Includes -
//================================================================================================================================

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
#include "combat_mng.h"
#include "game_array_simple.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "siege_snapshot.h"
#include "test_hlp_city_attack.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_chain_validation.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "war_turn_handler.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/war-turn-handler";
static const char* G_SIEGE_DIR = "/home/w/Projects/simple-map-gen/war-turn-handler/sieges";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/war-turn-handler/ownership.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/war-turn-handler/cities.txt";
static const char* G_UNITS = "/home/w/Projects/simple-map-gen/war-turn-handler/units.txt";
static const char* G_OWN_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/ownership.bin";
static const char* G_CITIES_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/cities.txt";
static const char* G_UNITS_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/units.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/war-turn-handler/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100u;
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

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool arg_has (int argc, char** argv, cstr flag) {
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && std::strcmp(argv[i], flag) == 0) {
            return true;
        }
    }
    return false;
}

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
    if (!(::mkdir(G_OUT_DIR, 0755) == 0 || errno == EEXIST)) {
        return false;
    }
    char turns[400];
    if (std::snprintf(turns, sizeof(turns), "%s/turns", G_OUT_DIR) <= 0) {
        return false;
    }
    return ::mkdir(turns, 0755) == 0 || errno == EEXIST;
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

static bool load_ownership_path (GameState& state, cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
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
    if (state.m_map.get_add_typ(x, y) == BUILD_ADD_CITY) {
        return state.m_cities.get_city(state.m_map.get_add_idx(x, y));
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_x() == x && c->get_y() == y) {
            return c;
        }
    }
    return nullptr;
}

static bool load_cities_path (GameState& state, cstr path) {
    std::FILE* fp = std::fopen(path, "r");
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
    return n > 0;
}

static u32 unit_scan_n () {
    return static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
}

static bool clear_all_units (GameState& state) {
    const u32 scan_n = unit_scan_n();
    for (;;) {
        bool any = false;
        for (u32 idx = 0; idx < scan_n; ++idx) {
            const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
            if (u == nullptr) {
                continue;
            }
            if (!UnitMovementMng::destroy_unit(state, UnitAddKey::from_raw(static_cast<u16>(idx)))) {
                return false;
            }
            any = true;
            break;
        }
        if (!any) {
            return true;
        }
    }
}

static bool write_unit_line (std::FILE* fp, const GameState& state, u16 player, u16 x, u16 y, u16 typ, u8 hp, u8 lvl) {
    if (fp == nullptr || state.m_statics == nullptr) {
        return false;
    }
    const char* nm = state.m_statics->unit().get_name(UnitStaticDataKey::from_raw(typ));
    if (nm == nullptr) {
        nm = "?";
    }
    return std::fprintf(fp, "%u:%u:%u:%s:%u:%u\n",
        static_cast<unsigned>(player),
        static_cast<unsigned>(x),
        static_cast<unsigned>(y),
        nm,
        static_cast<unsigned>(hp),
        static_cast<unsigned>(lvl)) > 0;
}

static bool save_units (const GameState& state) {
    if (state.m_statics == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(G_UNITS, "w");
    if (fp == nullptr) {
        return false;
    }
    const u32 scan_n = unit_scan_n();
    std::vector<u8> seen(scan_n, 0u);
    u32 n = 0;
    for (u32 idx = 0; idx < scan_n; ++idx) {
        if (seen[idx] != 0u) {
            continue;
        }
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u16 hx = static_cast<u16>(u->m_x);
        const u16 hy = static_cast<u16>(u->m_y);
        UnitAddKey cur = UnitAddKey::from_raw(static_cast<u16>(idx));
        while (cur.is_valid()) {
            const u16 raw = cur.value();
            if (raw < scan_n) {
                seen[raw] = 1u;
            }
            const UnitAddStruct* c = state.m_units.get_unit_add(cur);
            if (c == nullptr) {
                break;
            }
            if (!write_unit_line(fp, state, c->m_player_idx, hx, hy,
                    c->m_unit_typ_idx, c->m_health, c->m_level)) {
                std::fclose(fp);
                return false;
            }
            n++;
            if (c->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            cur = UnitAddKey::from_raw(c->m_next_unit_in_group);
        }
    }
    std::fclose(fp);
    std::printf("saved units=%u\n", static_cast<unsigned>(n));
    return n > 0;
}

static u16 find_unit_typ (const RuntimeStatics& st, cstr name) {
    if (name == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 un = st.unit().get_item_count();
    for (u16 i = 0; i < un; ++i) {
        cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool load_units_path (GameState& state, cstr path) {
    if (state.m_statics == nullptr) {
        std::printf("load_units: no statics\n");
        return false;
    }
    if (!clear_all_units(state)) {
        std::printf("load_units: clear failed\n");
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        std::printf("load_units: open failed %s\n", path);
        return false;
    }
    unsigned player = 0;
    unsigned x = 0;
    unsigned y = 0;
    unsigned hp = 0;
    unsigned lvl = 0;
    char name[128];
    u32 n = 0;
    u32 line = 0;
    u32 skipped = 0;
    while (std::fscanf(fp, "%u:%u:%u:%127[^:]:%u:%u\n", &player, &x, &y, name, &hp, &lvl) == 6) {
        line++;
        if (x >= state.m_map.width() || y >= state.m_map.height() || player >= state.m_player_n) {
            skipped++;
            continue;
        }
        const u16 typ = find_unit_typ(*state.m_statics, name);
        if (typ == U16_KEY_NULL) {
            std::printf("load_units: unknown typ '%s' line=%u\n", name, static_cast<unsigned>(line));
            std::fclose(fp);
            return false;
        }
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, static_cast<u16>(x), static_cast<u16>(y),
                static_cast<u16>(player), typ, &key)) {
            skipped++;
            continue;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            std::printf("load_units: null after place line=%u\n", static_cast<unsigned>(line));
            std::fclose(fp);
            return false;
        }
        u->m_health = static_cast<u8>(hp > 255u ? 255u : hp);
        u->m_level = static_cast<u8>(lvl > 255u ? 255u : lvl);
        n++;
    }
    std::fclose(fp);
    std::printf("loaded units=%u skipped=%u\n", static_cast<unsigned>(n), static_cast<unsigned>(skipped));
    return n > 0;
}

static void sync_city_tile_owners (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        state.m_map.set_civ_owner(c->get_x(), c->get_y(), static_cast<u8>(c->get_owner()));
    }
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
    return save_ownership(state) && save_cities(state) && save_units(state);
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

static bool write_owner_frame (
    cstr path,
    const GameState& state,
    const std::vector<u8>& cap,
    u8 attacker,
    u8 defender,
    u16 focus_x,
    u16 focus_y) {
    (void)cap;
    (void)focus_x;
    (void)focus_y;
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
        const u8* col = seat_rgb(static_cast<u8>(c->get_owner()), attacker, defender);
        paint_dot(rgb, w, h, c->get_x(), c->get_y(),
            static_cast<u8>(col[0] / 5u), static_cast<u8>(col[1] / 5u), static_cast<u8>(col[2] / 5u));
    }
    const u32 scan_n = unit_scan_n();
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (state.m_map.get_add_typ(u->m_x, u->m_y) == BUILD_ADD_CITY) {
            continue;
        }
        paint_dot(rgb, w, h, u->m_x, u->m_y, 255u, 255u, 255u);
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

static bool save_frame (
    const GameState& state,
    const std::vector<u8>& cap,
    u8 pa,
    u8 pb,
    u16 fx,
    u16 fy,
    u32 turn) {
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/turns/turn_%04u.ppm", G_OUT_DIR, static_cast<unsigned>(turn)) <= 0) {
        return false;
    }
    return write_owner_frame(path, state, cap, pa, pb, fx, fy);
}

static bool write_phase1_map (const GameState& state, u8 attacker, u8 defender) {
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/phase1.ppm", G_OUT_DIR) <= 0) {
        return false;
    }
    std::vector<u8> empty_cap;
    return write_owner_frame(path, state, empty_cap, attacker, defender, U16_KEY_NULL, U16_KEY_NULL);
}

static bool mark_captured (GameState& state, u16 x, u16 y, std::vector<u8>& cap) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_x() != x || c->get_y() != y) {
            continue;
        }
        if (i >= cap.size()) {
            cap.resize(static_cast<size_t>(i) + 1u, 0u);
        }
        cap[i] = 1u;
        return true;
    }
    return false;
}

static bool run_muster (
    GameState& state,
    WarTurnHandler& camp,
    std::vector<u8>& cap,
    u8 pa,
    u8 pb,
    u16 stx,
    u16 sty,
    bool chain_check,
    bool chain_print_all,
    u32* turn_io) {
    if (chain_check && !TestHlpUnitChainValidation::run(state, chain_print_all, nullptr)) {
        return false;
    }
    if (!save_frame(state, cap, pa, pb, stx, sty, *turn_io)) {
        return false;
    }
    (*turn_io)++;
    for (;;) {
        if (!camp.walk_muster()) {
            break;
        }
        if (chain_check && !TestHlpUnitChainValidation::run(state, chain_print_all, nullptr)) {
            return false;
        }
        if (!save_frame(state, cap, pa, pb, stx, sty, *turn_io)) {
            return false;
        }
        (*turn_io)++;
    }
    return true;
}

static bool run_march (
    GameState& state,
    WarTurnHandler& camp,
    std::vector<u8>& cap,
    u8 pa,
    u8 pb,
    u16 tx,
    u16 ty,
    bool chain_check,
    bool chain_print_all,
    u32* turn_io) {
    if (chain_check && !TestHlpUnitChainValidation::run(state, chain_print_all, nullptr)) {
        return false;
    }
    if (!save_frame(state, cap, pa, pb, tx, ty, *turn_io)) {
        return false;
    }
    (*turn_io)++;
    for (;;) {
        if (!camp.walk_army()) {
            break;
        }
        if (chain_check && !TestHlpUnitChainValidation::run(state, chain_print_all, nullptr)) {
            return false;
        }
        if (!save_frame(state, cap, pa, pb, tx, ty, *turn_io)) {
            return false;
        }
        (*turn_io)++;
    }
    return true;
}

static const u16 k_typ_cap = 256u;
static const u16 k_snap_cap = 2048u;

static u32 scan_unit_n () {
    return static_cast<u32>(UnitAddVector::MAX_PAGES)
        * static_cast<u32>(UnitAddVector::UNIT_ADD_ITEMS_PER_PAGE);
}

static u32 count_seat_units (const GameState& s, u16 seat, u16* by_typ, u16 typ_n) {
    if (by_typ != nullptr) {
        for (u16 t = 0; t < typ_n && t < k_typ_cap; ++t) {
            by_typ[t] = 0;
        }
    }
    u32 n = 0;
    const u32 scan_n = scan_unit_n();
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_player_idx != seat) {
            continue;
        }
        n++;
        if (by_typ != nullptr && u->m_unit_typ_idx < typ_n && u->m_unit_typ_idx < k_typ_cap) {
            by_typ[u->m_unit_typ_idx]++;
        }
    }
    return n;
}

static void print_seat_units (cstr label, const GameState& s, u16 seat) {
    if (s.m_statics == nullptr) {
        std::printf("%s seat=%u (no statics)\n", label, static_cast<unsigned>(seat));
        return;
    }
    const u16 typ_n = s.m_statics->unit().get_item_count();
    u16 by_typ[k_typ_cap];
    const u32 n = count_seat_units(s, seat, by_typ, typ_n);
    std::printf("%s seat=%u total=%u", label, static_cast<unsigned>(seat), static_cast<unsigned>(n));
    for (u16 t = 0; t < typ_n && t < k_typ_cap; ++t) {
        if (by_typ[t] == 0u) {
            continue;
        }
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(t));
        if (nm == nullptr) {
            nm = "?";
        }
        std::printf(" %s=%u", nm, static_cast<unsigned>(by_typ[t]));
    }
    std::printf("\n");
}

static u16 snap_tile (const GameState& s, u16 x, u16 y, u16 seat, u16* keys, u16* typs, u16 cap) {
    u16 n = 0;
    const u32 scan_n = scan_unit_n();
    for (u32 idx = 0; idx < scan_n && n < cap; ++idx) {
        const UnitAddStruct* u = s.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x != x || u->m_y != y || u->m_player_idx != seat) {
            continue;
        }
        keys[n] = static_cast<u16>(idx);
        typs[n] = u->m_unit_typ_idx;
        n++;
    }
    return n;
}

static u16 snap_grp (const GameState& s, UnitAddKey head, u16* keys, u16* typs, u16 cap) {
    u16 n = 0;
    UnitAddKey cur = head;
    while (cur.is_valid() && n < cap) {
        const UnitAddStruct* u = s.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        keys[n] = cur.value();
        typs[n] = u->m_unit_typ_idx;
        n++;
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    return n;
}

static u32 g_siege_i = 0;

static bool save_siege_snap (const GameState& state, UnitAddKey army, u16 cx, u16 cy, u16 def) {
    g_siege_i++;
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/%u.txt", G_SIEGE_DIR,
            static_cast<unsigned>(g_siege_i)) <= 0) {
        return false;
    }
    if (!SiegeSnapshot::save(state, army, cx, cy, def, path)) {
        return false;
    }
    std::printf("saved siege %u -> %s\n", static_cast<unsigned>(g_siege_i), path);
    return true;
}

static void print_army (cstr label, const GameState& s, UnitAddKey head) {
    u16 keys[k_snap_cap];
    u16 typs[k_snap_cap];
    const u16 n = snap_grp(s, head, keys, typs, k_snap_cap);
    u16 by_typ[k_typ_cap];
    for (u16 t = 0; t < k_typ_cap; ++t) {
        by_typ[t] = 0;
    }
    for (u16 i = 0; i < n; ++i) {
        if (typs[i] < k_typ_cap) {
            by_typ[typs[i]]++;
        }
    }
    std::printf("%s total=%u", label, static_cast<unsigned>(n));
    if (s.m_statics != nullptr) {
        const u16 typ_n = s.m_statics->unit().get_item_count();
        for (u16 t = 0; t < typ_n && t < k_typ_cap; ++t) {
            if (by_typ[t] == 0u) {
                continue;
            }
            const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(t));
            if (nm == nullptr) {
                nm = "?";
            }
            std::printf(" %s=%u", nm, static_cast<unsigned>(by_typ[t]));
        }
    }
    std::printf("\n");
}

static bool key_in (const u16* keys, u16 n, u16 key) {
    for (u16 i = 0; i < n; ++i) {
        if (keys[i] == key) {
            return true;
        }
    }
    return false;
}

static void fmt_lost (char* dst, size_t dst_n, const GameState& s, const u16* before_k, const u16* before_t,
    u16 bn, const u16* after_k, u16 an) {
    if (dst == nullptr || dst_n == 0u || s.m_statics == nullptr) {
        return;
    }
    dst[0] = '\0';
    u16 lct[k_typ_cap];
    for (u16 t = 0; t < k_typ_cap; ++t) {
        lct[t] = 0;
    }
    u16 lost = 0;
    for (u16 i = 0; i < bn; ++i) {
        if (key_in(after_k, an, before_k[i])) {
            continue;
        }
        lost++;
        if (before_t[i] < k_typ_cap) {
            lct[before_t[i]]++;
        }
    }
    size_t used = 0;
    const int n0 = std::snprintf(dst, dst_n, "lost=%u", static_cast<unsigned>(lost));
    if (n0 < 0) {
        return;
    }
    used = static_cast<size_t>(n0);
    const u16 typ_n = s.m_statics->unit().get_item_count();
    for (u16 t = 0; t < typ_n && t < k_typ_cap && used + 1u < dst_n; ++t) {
        if (lct[t] == 0u) {
            continue;
        }
        const char* nm = s.m_statics->unit().get_name(UnitStaticDataKey::from_raw(t));
        if (nm == nullptr) {
            nm = "?";
        }
        const int n1 = std::snprintf(dst + used, dst_n - used, " %s=%u", nm, static_cast<unsigned>(lct[t]));
        if (n1 < 0) {
            break;
        }
        used += static_cast<size_t>(n1);
    }
}

static void print_conquest (
    const GameState& s,
    WarTurnHandler& camp,
    u16 pa,
    u16 pb,
    u16 cx,
    u16 cy,
    u32 cap_n,
    const u16* atk_bk,
    const u16* atk_bt,
    u16 atk_bn,
    const u16* def_bk,
    const u16* def_bt,
    u16 def_bn) {
    u16 atk_ak[k_snap_cap];
    u16 atk_at[k_snap_cap];
    u16 def_ak[k_snap_cap];
    u16 def_at[k_snap_cap];
    u16 atk_an = 0;
    const UnitAddKey stay = UnitAddKey::from_raw(camp.split_hd(0));
    const UnitAddKey occ = UnitAddKey::from_raw(camp.atk_hd(0));
    if (stay.is_valid()) {
        atk_an = snap_grp(s, stay, atk_ak, atk_at, k_snap_cap);
    }
    if (occ.is_valid() && atk_an < k_snap_cap) {
        atk_an = static_cast<u16>(atk_an + snap_grp(s, occ, atk_ak + atk_an,
            atk_at + atk_an, static_cast<u16>(k_snap_cap - atk_an)));
    }
    (void)pa;
    const u16 def_an = snap_tile(s, cx, cy, pb, def_ak, def_at, k_snap_cap);
    char atk_lost[192];
    char def_lost[192];
    fmt_lost(atk_lost, sizeof(atk_lost), s, atk_bk, atk_bt, atk_bn, atk_ak, atk_an);
    fmt_lost(def_lost, sizeof(def_lost), s, def_bk, def_bt, def_bn, def_ak, def_an);
    std::printf("cap #%u (%u,%u)\n", static_cast<unsigned>(cap_n),
        static_cast<unsigned>(cx), static_cast<unsigned>(cy));
    std::printf("  barrage tot=%u last=%u\n",
        static_cast<unsigned>(camp.br_tot()), static_cast<unsigned>(camp.br_last()));
    std::printf("  atk before=%u after=%u %s\n",
        static_cast<unsigned>(atk_bn), static_cast<unsigned>(atk_an), atk_lost);
    std::printf("  def before=%u after=%u %s\n",
        static_cast<unsigned>(def_bn), static_cast<unsigned>(def_an), def_lost);
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    const bool c_check = arg_has(argc, argv, "--chain-check");
    const bool c_print_all = arg_has(argc, argv, "--chain-print-all");
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("*** FAILED path/out\n");
        return 1;
    }
    if (!SiegeSnapshot::ensure_dir(G_SIEGE_DIR)) {
        std::printf("*** FAILED siege dir\n");
        return 1;
    }
    {
        char cmd[512];
        if (std::snprintf(cmd, sizeof(cmd), "rm -f %s/*.txt", G_SIEGE_DIR) > 0) {
            (void)std::system(cmd);
        }
    }
    g_siege_i = 0;

    GameSetup setup;
    GameState state;
    MapPpmPaths paths = {};
    paths.m_terr = g_terr;
    paths.m_clim = g_clim;
    paths.m_riv = g_riv;
    paths.m_ov = g_ov;
    paths.m_res = g_res;
    if (!setup.setup_new_game(&state, paths, G_PLAYERS)) {
        std::printf("*** FAILED setup_new_game\n");
        return 1;
    }
    for (u16 i = 0; i < state.m_player_n; ++i) {
        state.m_player_states[i].m_ai_units = AiUnits::AI_UNITS_AGGRESSIVE;
    }
    CityBorder::bind_map(&state.m_map);
    if (state.m_statics == nullptr
        || !TileAttrTables::setup(*state.m_statics)
        || !UnitMovementMng::setup_mvt_costs(*state.m_statics)
        || !CombatMng::setup(*state.m_statics)) {
        std::printf("*** FAILED movement/combat setup\n");
        state.clear();
        return 1;
    }

    const bool have_cache = file_exists(G_OWN) && file_exists(G_CITIES) && file_exists(G_UNITS);
    const bool have_alt = file_exists(G_OWN_ALT) && file_exists(G_CITIES_ALT) && file_exists(G_UNITS_ALT);
    if (have_cache) {
        std::printf("cache hit: loading ownership + cities + units\n");
        if (!load_ownership_path(state, G_OWN)) {
            std::printf("*** FAILED load ownership\n");
            state.clear();
            return 1;
        }
        if (!load_cities_path(state, G_CITIES)) {
            std::printf("*** FAILED load cities\n");
            state.clear();
            return 1;
        }
        if (!load_units_path(state, G_UNITS)) {
            std::printf("*** FAILED load units\n");
            state.clear();
            return 1;
        }
    } else if (have_alt) {
        std::printf("cache hit (flood alt): loading ownership + cities + units\n");
        if (!load_ownership_path(state, G_OWN_ALT)
            || !load_cities_path(state, G_CITIES_ALT)
            || !load_units_path(state, G_UNITS_ALT)) {
            std::printf("*** FAILED alt cache load\n");
            state.clear();
            return 1;
        }
        if (!save_ownership(state) || !save_cities(state) || !save_units(state)) {
            std::printf("*** FAILED copy alt cache\n");
            state.clear();
            return 1;
        }
    } else {
        std::printf("cache miss: running %u turns with %u players\n", G_TURN_CAP, G_PLAYERS);
        if (!run_warmup(state)) {
            std::printf("*** FAILED warmup/save\n");
            state.clear();
            return 1;
        }
        std::printf("saved %s , %s , %s\n", G_OWN, G_CITIES, G_UNITS);
    }
    sync_city_tile_owners(state);
    if (!write_phase1_map(state, U8_KEY_NULL, U8_KEY_NULL)) {
        std::printf("*** FAILED write phase1.ppm\n");
        state.clear();
        return 1;
    }
    std::printf("wrote %s/phase1.ppm\n", G_OUT_DIR);

    u16 pa = 0;
    u16 pb = 0;
    if (!pick_seats(state, &pa, &pb)) {
        std::printf("*** FAILED pick_seats\n");
        state.clear();
        return 1;
    }
    u16 stx = 0;
    u16 sty = 0;
    if (!WarTurnHandler::pick_staging_city(state, pa, pb, &stx, &sty)) {
        std::printf("*** FAILED pick_staging_city\n");
        state.clear();
        return 1;
    }
    std::printf("attacker=%u defender=%u staging=(%u,%u) cities_A=%u cities_B=%u\n",
        static_cast<unsigned>(pa), static_cast<unsigned>(pb),
        static_cast<unsigned>(stx), static_cast<unsigned>(sty),
        static_cast<unsigned>(count_owner_cities(state, pa)),
        static_cast<unsigned>(count_owner_cities(state, pb)));
    print_seat_units("pre_units attacker", state, pa);
    print_seat_units("pre_units defender", state, pb);

    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    std::vector<u8> cap(state.m_cities.get_city_count(), 0u);
    WarTurnHandler camp(state, pa);
    if (!camp.ok() || !camp.make_muster_gradient(stx, sty)) {
        std::printf("*** FAILED make_muster_gradient staging=(%u,%u) owner=%u\n",
            static_cast<unsigned>(stx), static_cast<unsigned>(sty),
            static_cast<unsigned>(state.m_map.get_civ_owner(stx, sty)));
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }
    const u16 mn = camp.do_total_muster();
    std::printf("muster_groups=%u\n", static_cast<unsigned>(mn));
    if (!camp.determine_exposure(pb)) {
        std::printf("*** FAILED determine_exposure grp_n=%u\n", static_cast<unsigned>(camp.muster_n()));
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }

    u32 turn = 0;
    if (!run_muster(state, camp, cap, static_cast<u8>(pa), static_cast<u8>(pb), stx, sty,
            c_check, c_print_all, &turn)) {
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }
    if (!camp.form_army()) {
        std::printf("*** FAILED form_army\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }
    print_army("muster army", state, UnitAddKey::from_raw(camp.atk_hd(0)));
    if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), stx, sty, turn)) {
        std::printf("*** FAILED save form_army frame\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }
    turn++;

    u16 tx = 0;
    u16 ty = 0;
    u32 wave = 0;
    u32 captured = 0;
    if (!camp.set_target_city(pb, &tx, &ty)) {
        std::printf("*** FAILED set_target_city\n");
        WhiteboardMng::terminate();
        state.clear();
        return 1;
    }

    for (;;) {
        if (!run_march(state, camp, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty,
                c_check, c_print_all, &turn)) {
            std::printf("*** FAILED march to (%u,%u)\n", static_cast<unsigned>(tx), static_cast<unsigned>(ty));
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        u16 atk_bk[k_snap_cap];
        u16 atk_bt[k_snap_cap];
        u16 def_bk[k_snap_cap];
        u16 def_bt[k_snap_cap];
        const u16 atk_bn = snap_grp(state, UnitAddKey::from_raw(camp.atk_hd(0)),
            atk_bk, atk_bt, k_snap_cap);
        const u16 def_bn = snap_tile(state, tx, ty, pb, def_bk, def_bt, k_snap_cap);
        if (!save_siege_snap(state, UnitAddKey::from_raw(camp.atk_hd(0)), tx, ty, pb)) {
            std::printf("*** FAILED save siege snapshot\n");
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        WarAssault ar = TestHlpCityAttack::run(state, camp, tx, ty, false);
        while (ar == WarAssault::Stall) {
            std::printf("assault stall at (%u,%u); can_cont=%u barrage tot=%u last=%u\n",
                static_cast<unsigned>(tx), static_cast<unsigned>(ty),
                camp.can_cont(0u) ? 1u : 0u,
                static_cast<unsigned>(camp.br_tot()),
                static_cast<unsigned>(camp.br_last()));
            if (!camp.can_cont(0u)) {
                break;
            }
            if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty, turn)) {
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            turn++;
            if (!save_siege_snap(state, UnitAddKey::from_raw(camp.atk_hd(0)), tx, ty, pb)) {
                std::printf("*** FAILED save siege snapshot (stall retry)\n");
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            ar = TestHlpCityAttack::run(state, camp, tx, ty, false);
        }
        if (ar != WarAssault::Ok) {
            std::printf("war stop: assault %s at (%u,%u) captured=%u barrage tot=%u last=%u\n",
                camp.stalled() ? "stall" : "failed",
                static_cast<unsigned>(tx), static_cast<unsigned>(ty), captured,
                static_cast<unsigned>(camp.br_tot()),
                static_cast<unsigned>(camp.br_last()));
            print_army("remaining army", state, UnitAddKey::from_raw(camp.atk_hd(0)));
            break;
        }
        captured++;
        (void)mark_captured(state, tx, ty, cap);
        print_conquest(state, camp, pa, pb, tx, ty, captured,
            atk_bk, atk_bt, atk_bn, def_bk, def_bt, def_bn);
        std::fflush(stdout);
        if (c_check && !TestHlpUnitChainValidation::run(state, c_print_all, nullptr)) {
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty, turn)) {
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        turn++;

        const bool had_split = camp.split_hd(0) != U16_KEY_NULL;
        for (;;) {
            const u16 sh = camp.split_hd(0);
            if (sh == U16_KEY_NULL) {
                break;
            }
            const UnitAddStruct* occ = state.m_units.get_unit_add(UnitAddKey::from_raw(camp.atk_hd(0)));
            const UnitAddStruct* spl = state.m_units.get_unit_add(UnitAddKey::from_raw(sh));
            if (occ == nullptr || spl == nullptr || occ->m_x == U16_KEY_NULL || spl->m_x == U16_KEY_NULL) {
                std::printf("*** FAILED rejoin unit lookup\n");
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            if (occ->m_x == spl->m_x && occ->m_y == spl->m_y) {
                break;
            }
            if (!camp.rejoin_move(0u)) {
                std::printf("*** FAILED rejoin_move\n");
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty, turn)) {
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            turn++;
        }
        if (!camp.rejoin_link(0u)) {
            std::printf("*** FAILED rejoin_link\n");
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        if (had_split) {
            if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty, turn)) {
                WhiteboardMng::terminate();
                state.clear();
                return 1;
            }
            turn++;
        }
        {
            const u16 rested = camp.rest_heal(0u);
            if (rested > 0u) {
                std::printf("rest heal turns=%u\n", static_cast<unsigned>(rested));
                turn = static_cast<u32>(turn + rested);
                if (!save_frame(state, cap, static_cast<u8>(pa), static_cast<u8>(pb), tx, ty, turn)) {
                    WhiteboardMng::terminate();
                    state.clear();
                    return 1;
                }
                turn++;
            }
        }
        if (!camp.army_can_fight(0u)) {
            std::printf("war stop: no offensive units left captured=%u\n", captured);
            print_army("remaining army", state, UnitAddKey::from_raw(camp.atk_hd(0)));
            break;
        }
        wave++;
        if (!camp.set_target_city(pb, &tx, &ty)) {
            std::printf("war stop: no more enemy cities captured=%u\n", captured);
            print_army("remaining army", state, UnitAddKey::from_raw(camp.atk_hd(0)));
            break;
        }
        (void)wave;
    }

    std::printf("*** PASSED frames=%u captured=%u\n", turn, captured);
    WhiteboardMng::terminate();
    state.clear();
    setup.release_map_gen();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
