//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "bit_array.h"
#include "build_adds_array.h"
#include "building_static_key.h"
#include "city_array.h"
#include "city_flag_static_key.h"
#include "combat_mng.h"
#include "factory_game_array_simple.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "general_bit_bank.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "small_wonder_static_key.h"
#include "tech_static_key.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "wonder_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

typedef const char* cstr;

static RuntimeStaticLoader g_ldr;
static RuntimeStatics* g_st = nullptr;
static GameState g_gs;
static char g_err[256];

//================================================================================================================================
//=> - Config -
//================================================================================================================================

struct Cfg {
    char m_atk[64];
    char m_def[64];
    u8 m_atk_lv;
    u8 m_def_lv;
    u8 m_atk_hp;
    u8 m_def_hp;
    u8 m_terr;
    u8 m_ov;
    u8 m_riv;
    u8 m_city;
    char m_bld[256];
    char m_flag[256];
    char m_wond[256];
    char m_sw[256];
    char m_atk_tech[256];
    char m_def_tech[256];
};

static void cfg_def (Cfg* c) {
    std::memset(c, 0, sizeof(*c));
    std::snprintf(c->m_atk, sizeof(c->m_atk), "Horseman");
    std::snprintf(c->m_def, sizeof(c->m_def), "Spearman");
    c->m_atk_lv = GREEN;
    c->m_def_lv = GREEN;
    c->m_atk_hp = UNIT_HEALTH;
    c->m_def_hp = UNIT_HEALTH;
    c->m_terr = TERR_PLAINS[0];
    c->m_ov = OV_NONE[0];
    c->m_riv = 0u;
    c->m_city = 0u;
}

static void trim (char* s) {
    if (s == nullptr) {
        return;
    }
    char* a = s;
    while (*a == ' ' || *a == '\t') {
        ++a;
    }
    if (a != s) {
        std::memmove(s, a, std::strlen(a) + 1u);
    }
    size_t n = std::strlen(s);
    while (n > 0u && (s[n - 1u] == ' ' || s[n - 1u] == '\t' || s[n - 1u] == '\r' || s[n - 1u] == '\n')) {
        s[--n] = '\0';
    }
}

static bool parse_u8 (cstr s, u8* out) {
    if (s == nullptr || out == nullptr || *s == '\0') {
        return false;
    }
    unsigned v = 0u;
    if (std::sscanf(s, "%u", &v) != 1 || v > 255u) {
        return false;
    }
    *out = static_cast<u8>(v);
    return true;
}

static bool parse_lv (cstr s, u8* out) {
    if (std::strcmp(s, "VERY_GREEN") == 0) { *out = VERY_GREEN; return true; }
    if (std::strcmp(s, "GREEN") == 0) { *out = GREEN; return true; }
    if (std::strcmp(s, "REGULAR") == 0) { *out = REGULAR; return true; }
    if (std::strcmp(s, "DISCIPLINED") == 0) { *out = DISCIPLINED; return true; }
    if (std::strcmp(s, "HARDENED") == 0) { *out = HARDENED; return true; }
    if (std::strcmp(s, "VETERAN") == 0) { *out = VETERAN; return true; }
    if (std::strcmp(s, "COMMANDO") == 0) { *out = COMMANDO; return true; }
    if (std::strcmp(s, "ELITE") == 0) { *out = ELITE; return true; }
    return false;
}

static bool parse_terr (cstr s, u8* out) {
    if (std::strcmp(s, "plains") == 0) { *out = TERR_PLAINS[0]; return true; }
    if (std::strcmp(s, "hills") == 0) { *out = TERR_HILLS[0]; return true; }
    if (std::strcmp(s, "mountains") == 0) { *out = TERR_MOUNTAINS[0]; return true; }
    if (std::strcmp(s, "ocean") == 0) { *out = TERR_OCEAN[0]; return true; }
    if (std::strcmp(s, "sea") == 0) { *out = TERR_SEA[0]; return true; }
    if (std::strcmp(s, "coastal") == 0) { *out = TERR_COASTAL[0]; return true; }
    return false;
}

static bool parse_ov (cstr s, u8* out) {
    if (std::strcmp(s, "none") == 0) { *out = OV_NONE[0]; return true; }
    if (std::strcmp(s, "forest") == 0) { *out = OV_FOREST[0]; return true; }
    if (std::strcmp(s, "swamp") == 0) { *out = OV_SWAMP[0]; return true; }
    if (std::strcmp(s, "jungle") == 0) { *out = OV_JUNGLE[0]; return true; }
    return false;
}

static bool set_key (Cfg* c, cstr k, cstr v) {
    if (std::strcmp(k, "atk_unit") == 0) {
        std::snprintf(c->m_atk, sizeof(c->m_atk), "%s", v);
        return true;
    }
    if (std::strcmp(k, "def_unit") == 0) {
        std::snprintf(c->m_def, sizeof(c->m_def), "%s", v);
        return true;
    }
    if (std::strcmp(k, "atk_level") == 0) { return parse_lv(v, &c->m_atk_lv); }
    if (std::strcmp(k, "def_level") == 0) { return parse_lv(v, &c->m_def_lv); }
    if (std::strcmp(k, "atk_health") == 0) { return parse_u8(v, &c->m_atk_hp); }
    if (std::strcmp(k, "def_health") == 0) { return parse_u8(v, &c->m_def_hp); }
    if (std::strcmp(k, "terr") == 0) { return parse_terr(v, &c->m_terr); }
    if (std::strcmp(k, "ov") == 0) { return parse_ov(v, &c->m_ov); }
    if (std::strcmp(k, "riv") == 0) { return parse_u8(v, &c->m_riv); }
    if (std::strcmp(k, "city") == 0) { return parse_u8(v, &c->m_city); }
    if (std::strcmp(k, "bld") == 0) {
        std::snprintf(c->m_bld, sizeof(c->m_bld), "%s", v);
        return true;
    }
    if (std::strcmp(k, "flag") == 0) {
        std::snprintf(c->m_flag, sizeof(c->m_flag), "%s", v);
        return true;
    }
    if (std::strcmp(k, "wonder") == 0) {
        std::snprintf(c->m_wond, sizeof(c->m_wond), "%s", v);
        return true;
    }
    if (std::strcmp(k, "small_wonder") == 0) {
        std::snprintf(c->m_sw, sizeof(c->m_sw), "%s", v);
        return true;
    }
    if (std::strcmp(k, "atk_tech") == 0) {
        std::snprintf(c->m_atk_tech, sizeof(c->m_atk_tech), "%s", v);
        return true;
    }
    if (std::strcmp(k, "def_tech") == 0) {
        std::snprintf(c->m_def_tech, sizeof(c->m_def_tech), "%s", v);
        return true;
    }
    return false;
}

static bool load_diff (cstr path, Cfg* c) {
    FILE* f = std::fopen(path, "r");
    if (f == nullptr) {
        std::snprintf(g_err, sizeof(g_err), "open in failed");
        return false;
    }
    char line[512];
    while (std::fgets(line, sizeof(line), f) != nullptr) {
        trim(line);
        if (line[0] == '\0' || line[0] == '#') {
            continue;
        }
        char* sep = std::strstr(line, "==");
        if (sep == nullptr) {
            std::fclose(f);
            std::snprintf(g_err, sizeof(g_err), "bad line");
            return false;
        }
        *sep = '\0';
        char* val = sep + 2;
        trim(line);
        trim(val);
        if (!set_key(c, line, val)) {
            std::fclose(f);
            std::snprintf(g_err, sizeof(g_err), "bad key/val");
            return false;
        }
    }
    std::fclose(f);
    return true;
}

static bool write_out (cstr path, u32 wins, u32 samples) {
    FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "win_prob == %u\n", static_cast<unsigned>(wins));
    std::fprintf(f, "samples == %u\n", static_cast<unsigned>(samples));
    std::fclose(f);
    return true;
}

static bool write_err (cstr path) {
    FILE* f = std::fopen(path, "w");
    if (f == nullptr) {
        return false;
    }
    std::fprintf(f, "error == %s\n", g_err);
    std::fclose(f);
    return true;
}

//================================================================================================================================
//=> - Lookup -
//================================================================================================================================

static u16 find_unit (cstr name) {
    const UnitStaticData& ud = g_st->unit();
    const u16 n = ud.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(ud.get_name(UnitStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_bld (cstr name) {
    const u16 n = g_st->building().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(g_st->building().get_name(BuildingStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_flag (cstr name) {
    const u16 n = g_st->city_flag().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(g_st->city_flag().get_name(CityFlagStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_tech (cstr name) {
    const u16 n = g_st->tech().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(g_st->tech().get_name(TechStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_wond (cstr name) {
    const u16 n = g_st->wonder().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(g_st->wonder().get_name(WonderStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u16 find_sw (cstr name) {
    const u16 n = g_st->small_wonder().get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(g_st->small_wonder().get_name(SmallWonderStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool each_name (cstr list, bool (*fn) (cstr, void*), void* ud) {
    if (list == nullptr || list[0] == '\0') {
        return true;
    }
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s", list);
    char* p = buf;
    while (p != nullptr && *p != '\0') {
        char* comma = std::strchr(p, ',');
        if (comma != nullptr) {
            *comma = '\0';
        }
        trim(p);
        if (p[0] != '\0') {
            if (!fn(p, ud)) {
                return false;
            }
        }
        p = (comma != nullptr) ? (comma + 1) : nullptr;
    }
    return true;
}

struct ApplyTech {
    BitArrayCL* m_bits;
};

static bool on_tech (cstr nm, void* ud) {
    ApplyTech* a = static_cast<ApplyTech*>(ud);
    const u16 ix = find_tech(nm);
    if (ix == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown tech %s", nm);
        return false;
    }
    a->m_bits->set_bit(ix);
    return true;
}

struct ApplyBld {
    u16 m_city;
};

static bool on_bld (cstr nm, void* ud) {
    ApplyBld* a = static_cast<ApplyBld*>(ud);
    const u16 ix = find_bld(nm);
    if (ix == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown bld %s", nm);
        return false;
    }
    g_gs.m_cities.set_building_flag(a->m_city, ix);
    return true;
}

static bool on_flag (cstr nm, void* ud) {
    ApplyBld* a = static_cast<ApplyBld*>(ud);
    const u16 ix = find_flag(nm);
    if (ix == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown flag %s", nm);
        return false;
    }
    g_gs.m_cities.get_flag_bank()->set_flag(a->m_city, ix);
    return true;
}

static bool on_wond (cstr nm, void* ud) {
    ApplyBld* a = static_cast<ApplyBld*>(ud);
    const u16 ix = find_wond(nm);
    if (ix == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown wonder %s", nm);
        return false;
    }
    g_gs.m_wonder_city[ix] = a->m_city;
    return true;
}

static bool on_sw (cstr nm, void* ud) {
    ApplyBld* a = static_cast<ApplyBld*>(ud);
    const u16 ix = find_sw(nm);
    if (ix == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown small_wonder %s", nm);
        return false;
    }
    g_gs.m_player_states[1].m_small_wonder_city[ix] = a->m_city;
    return true;
}

//================================================================================================================================
//=> - Scene -
//================================================================================================================================

static bool mk_players () {
    const u16 tech_n = g_st->tech().get_item_count();
    const u16 sw_n = g_st->small_wonder().get_item_count();
    const u16 wond_n = g_st->wonder().get_item_count();
    PlayerState* seats = new PlayerState[2];
    for (u16 i = 0; i < 2u; ++i) {
        seats[i].m_techs_researched = new BitArrayCL(tech_n);
        seats[i].m_small_wonder_city = new u16[sw_n];
        for (u16 j = 0; j < sw_n; ++j) {
            seats[i].m_small_wonder_city[j] = U16_KEY_NULL;
        }
        seats[i].m_is_active = 1;
        seats[i].m_civ_index = i;
    }
    g_gs.m_player_states = seats;
    g_gs.m_player_n = 2u;
    g_gs.m_players_remaining = 2u;
    g_gs.m_small_wonder_count = sw_n;
    g_gs.m_wonder_count = wond_n;
    g_gs.m_wonder_city = new u16[wond_n];
    for (u16 i = 0; i < wond_n; ++i) {
        g_gs.m_wonder_city[i] = U16_KEY_NULL;
    }
    return true;
}

static bool apply_scene (const Cfg& c) {
    if (!Factory_GameArraySimple::init_test_grid(&g_gs.m_map, 2u, 1u)) {
        std::snprintf(g_err, sizeof(g_err), "map init failed");
        return false;
    }
    for (u16 x = 0; x < 2u; ++x) {
        GameTileSimple* t = g_gs.m_map.tile(x, 0u);
        t->m_clim = CLIMATE_NONE;
        t->m_road_typ = ROAD_NONE;
        t->m_terr = c.m_terr;
        t->m_ov = c.m_ov;
        t->m_riv = c.m_riv;
    }
    if (!g_gs.m_cities.bind_statics(*g_st)) {
        std::snprintf(g_err, sizeof(g_err), "cities bind failed");
        return false;
    }
    if (!mk_players()) {
        std::snprintf(g_err, sizeof(g_err), "players failed");
        return false;
    }
    ApplyTech ta = { g_gs.m_player_states[0].m_techs_researched };
    ApplyTech td = { g_gs.m_player_states[1].m_techs_researched };
    if (!each_name(c.m_atk_tech, on_tech, &ta) || !each_name(c.m_def_tech, on_tech, &td)) {
        return false;
    }
    u16 city_idx = U16_KEY_NULL;
    if (c.m_city != 0u) {
        city_idx = g_gs.m_cities.get_next_new_city_idx();
        if (!g_gs.m_map.set_tile_add(1u, 0u, city_idx, BUILD_ADD_CITY)
            || !g_gs.m_map.set_civ_owner(1u, 0u, 1u)) {
            std::snprintf(g_err, sizeof(g_err), "city tile failed");
            return false;
        }
        ApplyBld ab = { city_idx };
        if (!each_name(c.m_bld, on_bld, &ab)
            || !each_name(c.m_flag, on_flag, &ab)
            || !each_name(c.m_wond, on_wond, &ab)
            || !each_name(c.m_sw, on_sw, &ab)) {
            return false;
        }
    }
    return true;
}

static UnitAddStruct mk_unit (u16 typ, u16 x, u16 y, u8 seat, u8 lv, u8 hp) {
    UnitAddStruct u = {};
    u.m_x = x;
    u.m_y = y;
    u.m_unit_typ_idx = typ;
    u.m_next_unit_on_tile = U16_KEY_NULL;
    u.m_next_unit_in_group = U16_KEY_NULL;
    u.m_mvt_points = 0;
    u.m_player_idx = seat;
    u.m_health = hp;
    u.m_level = lv;
    u.m_misc = 0u;
    return u;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    g_err[0] = '\0';
    if (argc != 3) {
        std::snprintf(g_err, sizeof(g_err), "usage: probe in.trace out.trace");
        if (argc >= 3) {
            write_err(argv[2]);
        }
        return 1;
    }
    const cstr in_path = argv[1];
    const cstr out_path = argv[2];
    Cfg cfg;
    cfg_def(&cfg);
    if (!load_diff(in_path, &cfg)) {
        write_err(out_path);
        return 1;
    }
    if (!g_ldr.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        std::snprintf(g_err, sizeof(g_err), "statics load failed");
        write_err(out_path);
        return 1;
    }
    g_st = &g_ldr.statics();
    g_gs.m_statics = g_st;
    if (!TileAttrTables::setup(*g_st)
        || !CombatMng::setup(*g_st)
        || !g_gs.m_combat_mods.setup(*g_st)) {
        std::snprintf(g_err, sizeof(g_err), "combat setup failed");
        write_err(out_path);
        return 1;
    }
    CombatMng::set_dials(85u, 25u, 1u);
    if (!apply_scene(cfg)) {
        write_err(out_path);
        return 1;
    }
    const u16 atk_t = find_unit(cfg.m_atk);
    const u16 def_t = find_unit(cfg.m_def);
    if (atk_t == U16_KEY_NULL || def_t == U16_KEY_NULL) {
        std::snprintf(g_err, sizeof(g_err), "unknown unit");
        write_err(out_path);
        return 1;
    }
    UnitAddStruct atk = mk_unit(atk_t, 0u, 0u, 0u, cfg.m_atk_lv, cfg.m_atk_hp);
    UnitAddStruct def = mk_unit(def_t, 1u, 0u, 1u, cfg.m_def_lv, cfg.m_def_hp);
    static const u32 k_n = 10000u;
    u32 wins = 0u;
    for (u32 i = 0; i < k_n; ++i) {
        UnitAddStruct a = atk;
        UnitAddStruct d = def;
        CombatMng::resolve(a, d, g_gs, 1u, 0u);
        if (d.m_health == 0u && a.m_health > 0u) {
            wins++;
        }
    }
    CombatMng::clear();
    TileAttrTables::clear();
    if (!write_out(out_path, wins, k_n)) {
        return 1;
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
