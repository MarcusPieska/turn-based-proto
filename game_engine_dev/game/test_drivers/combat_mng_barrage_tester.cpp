//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "city_attack_manager.h"
#include "building_static_key.h"
#include "city.h"
#include "city_array.h"
#include "city_border.h"
#include "city_defense_booster_register.h"
#include "combat_mng.h"
#include "effect_ctx.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "general_bit_bank.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/combat-mng-barrage";
static const char* G_OWN = "/home/w/Projects/simple-map-gen/war-turn-handler/ownership.bin";
static const char* G_CITIES = "/home/w/Projects/simple-map-gen/war-turn-handler/cities.txt";
static const char* G_UNITS = "/home/w/Projects/simple-map-gen/war-turn-handler/units.txt";
static const char* G_OWN_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/ownership.bin";
static const char* G_CITIES_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/cities.txt";
static const char* G_UNITS_ALT = "/home/w/Projects/simple-map-gen/target-ordering-flood/units.txt";
static const char* G_OWN_OUT = "/home/w/Projects/simple-map-gen/combat-mng-barrage/ownership.bin";
static const char* G_CITIES_OUT = "/home/w/Projects/simple-map-gen/combat-mng-barrage/cities.txt";
static const char* G_UNITS_OUT = "/home/w/Projects/simple-map-gen/combat-mng-barrage/units.txt";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/combat-mng-barrage/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100u;
static const u32 G_TURN_CAP = 300u;
static const u16 G_DEF_N = 10u;
static const u16 G_DEF_MECH_N = 5u;
static const u16 G_DEF_MAR_N = 5u;
static const u16 G_ATK_N = 24u;

static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
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

static bool save_ownership_path (const GameState& state, cstr path) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    std::FILE* fp = std::fopen(path, "wb");
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
        if (!found_city(state, static_cast<u16>(x), static_cast<u16>(y), static_cast<u16>(own))) {
            std::fclose(fp);
            return false;
        }
        City* c = city_at(state, static_cast<u16>(x), static_cast<u16>(y));
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

static bool save_cities_path (const GameState& state, cstr path) {
    std::FILE* fp = std::fopen(path, "w");
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
        return false;
    }
    if (!clear_all_units(state)) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    unsigned player = 0;
    unsigned x = 0;
    unsigned y = 0;
    unsigned hp = 0;
    unsigned lvl = 0;
    char name[128];
    u32 n = 0;
    while (std::fscanf(fp, "%u:%u:%u:%127[^:]:%u:%u\n", &player, &x, &y, name, &hp, &lvl) == 6) {
        if (x >= state.m_map.width() || y >= state.m_map.height() || player >= state.m_player_n) {
            continue;
        }
        const u16 typ = find_unit_typ(*state.m_statics, name);
        if (typ == U16_KEY_NULL) {
            std::fclose(fp);
            return false;
        }
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, static_cast<u16>(x), static_cast<u16>(y),
                static_cast<u16>(player), typ, &key)) {
            continue;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            std::fclose(fp);
            return false;
        }
        u->m_health = static_cast<u8>(hp > 255u ? 255u : hp);
        u->m_level = static_cast<u8>(lvl > 255u ? 255u : lvl);
        n++;
    }
    std::fclose(fp);
    return n > 0;
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

static bool save_units_path (const GameState& state, cstr path) {
    if (state.m_statics == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "w");
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
    return save_ownership_path(state, G_OWN_OUT)
        && save_cities_path(state, G_CITIES_OUT)
        && save_units_path(state, G_UNITS_OUT);
}

static bool is_land (const GameState& state, u16 x, u16 y) {
    if (x >= state.m_map.width() || y >= state.m_map.height()) {
        return false;
    }
    return !overlay_is_water_terr(state.m_map.get_terrain(x, y));
}

static bool find_adj_land (const GameState& state, u16 cx, u16 cy, u16* ox, u16* oy) {
    static const i16 k_dx[4] = {1, -1, 0, 0};
    static const i16 k_dy[4] = {0, 0, 1, -1};
    for (u16 i = 0; i < 4u; ++i) {
        const i32 nx = static_cast<i32>(cx) + k_dx[i];
        const i32 ny = static_cast<i32>(cy) + k_dy[i];
        if (nx < 0 || ny < 0) {
            continue;
        }
        const u16 x = static_cast<u16>(nx);
        const u16 y = static_cast<u16>(ny);
        if (!is_land(state, x, y)) {
            continue;
        }
        *ox = x;
        *oy = y;
        return true;
    }
    return false;
}

static bool pick_city (GameState& state, u16* cx, u16* cy, u16* def, u16* ax, u16* ay) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        u16 x = 0;
        u16 y = 0;
        if (!find_adj_land(state, c->get_x(), c->get_y(), &x, &y)) {
            continue;
        }
        *cx = c->get_x();
        *cy = c->get_y();
        *def = c->get_owner();
        *ax = x;
        *ay = y;
        return true;
    }
    return false;
}

static u16 pick_atk_seat (const GameState& state, u16 def) {
    for (u16 i = 0; i < state.m_player_n; ++i) {
        if (i != def) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool clear_tile (GameState& state, u16 x, u16 y) {
    for (;;) {
        const u16 hd = state.m_map.get_unit_hd(x, y);
        if (hd == U16_KEY_NULL) {
            return true;
        }
        if (!UnitMovementMng::destroy_unit(state, UnitAddKey::from_raw(hd))) {
            return false;
        }
    }
}

static bool spawn_n (GameState& state, u16 x, u16 y, u16 seat, u16 typ, u16 n, UnitAddKey* head) {
    for (u16 i = 0; i < n; ++i) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, x, y, seat, typ, &key)) {
            return false;
        }
        if (!head->is_valid()) {
            *head = key;
        } else if (!UnitMovementMng::link_group(state, *head, key)) {
            return false;
        }
    }
    return true;
}

static u16 find_bld_typ (const RuntimeStatics& st, cstr name) {
    if (name == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 n = st.building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = st.building().get_name(BuildingStaticDataKey::from_raw(i));
        if (nm != nullptr && std::strcmp(nm, name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool give_walls (GameState& state, u16 cx, u16 cy) {
    if (state.m_map.get_add_typ(cx, cy) != BUILD_ADD_CITY || state.m_statics == nullptr) {
        return false;
    }
    const u16 walls = find_bld_typ(*state.m_statics, "Walls");
    if (walls == U16_KEY_NULL) {
        return false;
    }
    const u16 city_idx = state.m_map.get_add_idx(cx, cy);
    City* city = state.m_cities.get_city(city_idx);
    GeneralBitBank* bank = state.m_cities.get_bld_bank();
    if (city == nullptr || bank == nullptr) {
        return false;
    }
    bank->set_flag(city_idx, walls);
    city->set_defense_deduction(0u);
    return true;
}

static u16 city_def_boost (const GameState& state, u16 cx, u16 cy) {
    if (state.m_map.get_add_typ(cx, cy) != BUILD_ADD_CITY) {
        return 0u;
    }
    const u16 city_idx = state.m_map.get_add_idx(cx, cy);
    EffectCtx ctx = {};
    ctx.m_owner = static_cast<u16>(state.m_map.get_civ_owner(cx, cy));
    ctx.m_city_idx = city_idx;
    if (state.m_player_states != nullptr && ctx.m_owner < state.m_player_n) {
        ctx.m_tech = state.m_player_states[ctx.m_owner].m_techs_researched;
        ctx.m_small_wonder_city = state.m_player_states[ctx.m_owner].m_small_wonder_city;
    }
    ctx.m_bld_bank = state.m_cities.get_bld_bank();
    ctx.m_wonder_city = state.m_wonder_city;
    ctx.m_wonder_n = state.m_wonder_count;
    ctx.m_small_wonder_n = state.m_small_wonder_count;
    const i16 boost = CityDefenseBoosterRegister::determine_effect(ctx).m_perc;
    return (boost > 0) ? static_cast<u16>(boost) : 0u;
}

static void print_city_def (const GameState& state, u16 cx, u16 cy, cstr ind) {
    const u16 boost = city_def_boost(state, cx, cy);
    u16 ded = 0u;
    if (state.m_map.get_add_typ(cx, cy) == BUILD_ADD_CITY) {
        const City* city = state.m_cities.get_city(state.m_map.get_add_idx(cx, cy));
        if (city != nullptr) {
            ded = city->get_defense_deduction();
        }
    }
    const u16 eff = (ded < boost) ? static_cast<u16>(boost - ded) : 0u;
    std::printf("%scity_def boost=%u ded=%u eff=%u absorb=%u\n",
        ind,
        static_cast<unsigned>(boost),
        static_cast<unsigned>(ded),
        static_cast<unsigned>(eff),
        (ded < boost) ? 1u : 0u);
}

static void print_def_hp (const GameState& state, u16 x, u16 y, u16 def_seat, cstr ind) {
    std::printf("%shp:", ind);
    bool first = true;
    u16 cur = state.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        UnitAddKey g = UnitAddKey::from_raw(cur);
        while (g.is_valid()) {
            const UnitAddStruct* gu = state.m_units.get_unit_add(g);
            if (gu == nullptr) {
                break;
            }
            if (gu->m_player_idx == def_seat) {
                std::printf(first ? " %u" : ", %u", static_cast<unsigned>(gu->m_health));
                first = false;
            }
            if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
        }
        cur = next_tile;
    }
    std::printf("\n");
}

static bool build_scenario (
    GameState& state,
    u16 cx,
    u16 cy,
    u16 def,
    u16 ax,
    u16 ay,
    u16 atk,
    UnitAddKey* army)
{
    const u16 mech = find_unit_typ(*state.m_statics, "Mech Infantry");
    const u16 inf = find_unit_typ(*state.m_statics, "Infantry");
    const u16 mar = find_unit_typ(*state.m_statics, "Marine");
    const u16 rad = find_unit_typ(*state.m_statics, "Radar Artillery");
    if (mech == U16_KEY_NULL || inf == U16_KEY_NULL || mar == U16_KEY_NULL || rad == U16_KEY_NULL) {
        return false;
    }
    if (!clear_tile(state, cx, cy) || !clear_tile(state, ax, ay)) {
        return false;
    }
    if (!give_walls(state, cx, cy)) {
        return false;
    }
    UnitAddKey def_hd = UnitAddKey::None();
    if (!spawn_n(state, cx, cy, def, mech, G_DEF_MECH_N, &def_hd)) {
        return false;
    }
    if (!spawn_n(state, cx, cy, def, mar, G_DEF_MAR_N, &def_hd)) {
        return false;
    }
    UnitAddKey atk_hd = UnitAddKey::None();
    const u16 rad_n = G_ATK_N / 2u;
    const u16 mar_n = G_ATK_N / 4u;
    const u16 inf_n = G_ATK_N - rad_n - mar_n;
    if (!spawn_n(state, ax, ay, atk, rad, rad_n, &atk_hd)) {
        return false;
    }
    if (!spawn_n(state, ax, ay, atk, mar, mar_n, &atk_hd)) {
        return false;
    }
    if (!spawn_n(state, ax, ay, atk, inf, inf_n, &atk_hd)) {
        return false;
    }
    *army = atk_hd;
    return army->is_valid();
}

struct AtkRec {
    UnitAddKey m_key;
    u16 m_typ;
};

static void collect_atk (GameState& state, UnitAddKey army, std::vector<AtkRec>* out) {
    out->clear();
    UnitAddKey cur = army;
    while (cur.is_valid()) {
        UnitAddStruct* u = state.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        AtkRec r;
        r.m_key = cur;
        r.m_typ = u->m_unit_typ_idx;
        out->push_back(r);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
}

static void print_atk_outcome (const GameState& state, const std::vector<AtkRec>& recs, cstr ind) {
    if (state.m_statics == nullptr) {
        return;
    }
    u16 last_typ = U16_KEY_NULL;
    bool line_open = false;
    for (u32 i = 0; i < recs.size(); ++i) {
        const AtkRec& r = recs[i];
        const UnitStaticDataKey tk = UnitStaticDataKey::from_raw(r.m_typ);
        cstr nm = state.m_statics->unit().get_name(tk);
        if (nm == nullptr) {
            nm = "?";
        }
        const UnitAddStruct* u = state.m_units.get_unit_add(r.m_key);
        const unsigned hp = (u != nullptr) ? static_cast<unsigned>(u->m_health) : 0u;
        if (!line_open || r.m_typ != last_typ) {
            if (line_open) {
                std::printf("\n");
            }
            std::printf("%s%s: %u", ind, nm, hp);
            line_open = true;
            last_typ = r.m_typ;
        } else {
            std::printf(", %u", hp);
        }
    }
    if (line_open) {
        std::printf("\n");
    }
}

static void print_def_breakdown (const GameState& state, u16 x, u16 y, u16 def_seat) {
    if (state.m_statics == nullptr) {
        return;
    }
    std::printf("defenders:\n");
    u16 last_typ = U16_KEY_NULL;
    u16 n = 0;
    u16 def_stat = 0;
    cstr nm = "?";
    u16 cur = state.m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        UnitAddKey g = UnitAddKey::from_raw(cur);
        while (g.is_valid()) {
            const UnitAddStruct* gu = state.m_units.get_unit_add(g);
            if (gu == nullptr) {
                break;
            }
            if (gu->m_player_idx == def_seat) {
                if (n > 0u && gu->m_unit_typ_idx != last_typ) {
                    std::printf("  %s defense=%u n=%u\n",
                        nm, static_cast<unsigned>(def_stat), static_cast<unsigned>(n));
                    n = 0;
                }
                const UnitStaticDataKey tk = UnitStaticDataKey::from_raw(gu->m_unit_typ_idx);
                nm = state.m_statics->unit().get_name(tk);
                if (nm == nullptr) {
                    nm = "?";
                }
                def_stat = state.m_statics->unit().get_item(tk).defense;
                last_typ = gu->m_unit_typ_idx;
                n++;
            }
            if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                break;
            }
            g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
        }
        cur = next_tile;
    }
    if (n > 0u) {
        std::printf("  %s defense=%u n=%u\n",
            nm, static_cast<unsigned>(def_stat), static_cast<unsigned>(n));
    }
}

static bool run_case (
    GameState& state,
    u16 cx,
    u16 cy,
    u16 def,
    u16 ax,
    u16 ay,
    u16 atk,
    u16 bar_n,
    cstr label)
{
    UnitAddKey army = UnitAddKey::None();
    if (!build_scenario(state, cx, cy, def, ax, ay, atk, &army)) {
        std::printf("*** FAILED build_scenario (%s)\n", label);
        return false;
    }
    std::vector<AtkRec> recs;
    collect_atk(state, army, &recs);
    std::printf("%s\n", label);
    print_city_def(state, cx, cy, "  ");
    print_def_hp(state, cx, cy, def, "  ");
    for (u16 t = 0; t < bar_n; ++t) {
        CityAttackManager::refill_mp(state, army);
        CityBarrage br = {};
        if (!CityAttackManager::barrage(state, army, cx, cy, &br)) {
            std::printf("*** FAILED barrage (%s) turn=%u\n", label, static_cast<unsigned>(t));
            return false;
        }
        std::printf("  after barrage %u tot=%u\n",
            static_cast<unsigned>(t), static_cast<unsigned>(br.m_tot));
        print_city_def(state, cx, cy, "    ");
        print_def_hp(state, cx, cy, def, "    ");
    }
    CityAttackManager::refill_mp(state, army);
    UnitAddKey stay = UnitAddKey::None();
    UnitAddKey occ = UnitAddKey::None();
    (void)CityAttackManager::melee(state, &army, cx, cy, &stay, &occ);
    std::printf("  after melee\n");
    print_atk_outcome(state, recs, "    ");
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!build_paths() || !ensure_out_dir()) {
        std::printf("*** FAILED path/out\n");
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
        std::printf("*** FAILED setup_new_game\n");
        return 1;
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
    const bool have_out = file_exists(G_OWN_OUT) && file_exists(G_CITIES_OUT) && file_exists(G_UNITS_OUT);
    if (have_cache) {
        std::printf("cache hit: loading war-turn-handler ownership + cities + units\n");
        if (!load_ownership_path(state, G_OWN)
            || !load_cities_path(state, G_CITIES)
            || !load_units_path(state, G_UNITS)) {
            std::printf("*** FAILED cache load\n");
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
    } else if (have_out) {
        std::printf("cache hit (local): loading combat-mng-barrage ownership + cities + units\n");
        if (!load_ownership_path(state, G_OWN_OUT)
            || !load_cities_path(state, G_CITIES_OUT)
            || !load_units_path(state, G_UNITS_OUT)) {
            std::printf("*** FAILED local cache load\n");
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
    }
    sync_city_tile_owners(state);

    u16 cx = 0;
    u16 cy = 0;
    u16 def = 0;
    u16 ax = 0;
    u16 ay = 0;
    if (!pick_city(state, &cx, &cy, &def, &ax, &ay)) {
        std::printf("*** FAILED pick_city\n");
        state.clear();
        return 1;
    }
    const u16 atk = pick_atk_seat(state, def);
    if (atk == U16_KEY_NULL) {
        std::printf("*** FAILED pick_atk_seat\n");
        state.clear();
        return 1;
    }
    UnitAddKey army = UnitAddKey::None();
    if (!build_scenario(state, cx, cy, def, ax, ay, atk, &army)) {
        std::printf("*** FAILED build_scenario city=(%u,%u) adj=(%u,%u)\n",
            static_cast<unsigned>(cx), static_cast<unsigned>(cy),
            static_cast<unsigned>(ax), static_cast<unsigned>(ay));
        state.clear();
        return 1;
    }
    std::printf("city=(%u,%u) def=%u atk=%u adj=(%u,%u) def_n=%u atk_n=%u\n",
        static_cast<unsigned>(cx), static_cast<unsigned>(cy),
        static_cast<unsigned>(def), static_cast<unsigned>(atk),
        static_cast<unsigned>(ax), static_cast<unsigned>(ay),
        static_cast<unsigned>(G_DEF_N), static_cast<unsigned>(G_ATK_N));
    print_def_breakdown(state, cx, cy, def);

    if (!run_case(state, cx, cy, def, ax, ay, atk, 0u, "no barrage")
        || !run_case(state, cx, cy, def, ax, ay, atk, 1u, "1 turn barrage")
        || !run_case(state, cx, cy, def, ax, ay, atk, 2u, "2 turn barrage")) {
        state.clear();
        return 1;
    }

    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
