//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>
#include <vector>

#include "building_static_key.h"
#include "city_attack_manager.h"
#include "siege_snapshot.h"
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
    return save_ownership_path(state, G_OWN)
        && save_cities_path(state, G_CITIES)
        && save_units_path(state, G_UNITS);
}


//================================================================================================================================
//=> - Print helpers -
//================================================================================================================================

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

struct UnitCnt {
    u16 m_typ;
    u8 m_lvl;
    u8 m_hp;
    u16 m_n;
};

static const char* unit_nm (const GameState& state, u16 typ) {
    if (state.m_statics == nullptr) {
        return "?";
    }
    const char* n = state.m_statics->unit().get_name(UnitStaticDataKey::from_raw(typ));
    return (n != nullptr) ? n : "?";
}

static void print_cnts (cstr ind, const GameState& state, const UnitCnt* rows, u16 n) {
    if (n == 0u) {
        std::printf("%s(none)\n", ind);
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        std::printf("%s%u %s lvl=%u hp=%u\n",
            ind,
            static_cast<unsigned>(rows[i].m_n),
            unit_nm(state, rows[i].m_typ),
            static_cast<unsigned>(rows[i].m_lvl),
            static_cast<unsigned>(rows[i].m_hp));
    }
}

static bool add_cnt (UnitCnt* rows, u16* n, u16 cap, u16 typ, u8 lvl, u8 hp) {
    for (u16 i = 0; i < *n; ++i) {
        if (rows[i].m_typ == typ && rows[i].m_lvl == lvl && rows[i].m_hp == hp) {
            rows[i].m_n++;
            return true;
        }
    }
    if (*n >= cap) {
        return false;
    }
    rows[*n].m_typ = typ;
    rows[*n].m_lvl = lvl;
    rows[*n].m_hp = hp;
    rows[*n].m_n = 1u;
    (*n)++;
    return true;
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

static void print_grp (cstr ind, const GameState& state, UnitAddKey head) {
    UnitCnt rows[256];
    u16 n = 0;
    UnitAddKey cur = head;
    while (cur.is_valid()) {
        const UnitAddStruct* u = state.m_units.get_unit_add(cur);
        if (u == nullptr) {
            break;
        }
        (void)add_cnt(rows, &n, 256u, u->m_unit_typ_idx, u->m_level, u->m_health);
        if (u->m_next_unit_in_group == U16_KEY_NULL) {
            break;
        }
        cur = UnitAddKey::from_raw(u->m_next_unit_in_group);
    }
    print_cnts(ind, state, rows, n);
}

static void print_tile_seat (cstr ind, const GameState& state, u16 x, u16 y, u16 seat) {
    UnitCnt rows[256];
    u16 n = 0;
    const u32 scan_n = unit_scan_n();
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x != x || u->m_y != y || u->m_player_idx != seat) {
            continue;
        }
        (void)add_cnt(rows, &n, 256u, u->m_unit_typ_idx, u->m_level, u->m_health);
    }
    print_cnts(ind, state, rows, n);
}

static u32 list_sieges (u32* out, u32 cap) {
    u32 n = 0;
    for (u32 i = 1; i < 10000u && n < cap; ++i) {
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/%u.txt", G_SIEGE_DIR, static_cast<unsigned>(i)) <= 0) {
            break;
        }
        if (!file_exists(path)) {
            break;
        }
        out[n++] = i;
    }
    return n;
}

static bool replay_siege (GameState& state, u32 idx) {
    char path[512];
    if (std::snprintf(path, sizeof(path), "%s/%u.txt", G_SIEGE_DIR, static_cast<unsigned>(idx)) <= 0) {
        return false;
    }
    if (!file_exists(path)) {
        std::printf("*** missing siege %u (%s)\n", static_cast<unsigned>(idx), path);
        return false;
    }
    UnitAddKey army = UnitAddKey::None();
    u16 cx = 0;
    u16 cy = 0;
    u16 def = 0;
    if (!SiegeSnapshot::apply(state, path, &army, &cx, &cy, &def)) {
        std::printf("*** FAILED apply siege %u\n", static_cast<unsigned>(idx));
        return false;
    }
    std::printf("siege %u city=(%u,%u) def=%u\n",
        static_cast<unsigned>(idx),
        static_cast<unsigned>(cx),
        static_cast<unsigned>(cy),
        static_cast<unsigned>(def));

    std::printf("setup\n");
    print_city_def(state, cx, cy, "  ");
    std::printf("  army\n");
    print_grp("    ", state, army);
    std::printf("  def\n");
    print_tile_seat("    ", state, cx, cy, def);

    std::printf("barrage 1\n");
    std::printf("  before\n");
    print_city_def(state, cx, cy, "    ");
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);
    CityAttackManager::refill_mp(state, army);
    CityBarrage br1 = {};
    if (!CityAttackManager::barrage(state, army, cx, cy, &br1)) {
        std::printf("*** FAILED barrage 1\n");
        return false;
    }
    std::printf("  after tot=%u last=%u\n", static_cast<unsigned>(br1.m_tot), static_cast<unsigned>(br1.m_last));
    print_city_def(state, cx, cy, "    ");
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);

    std::printf("barrage 2\n");
    std::printf("  before\n");
    print_city_def(state, cx, cy, "    ");
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);
    CityAttackManager::refill_mp(state, army);
    CityBarrage br2 = {};
    if (!CityAttackManager::barrage(state, army, cx, cy, &br2)) {
        std::printf("*** FAILED barrage 2\n");
        return false;
    }
    std::printf("  after tot=%u last=%u\n", static_cast<unsigned>(br2.m_tot), static_cast<unsigned>(br2.m_last));
    print_city_def(state, cx, cy, "    ");
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);

    std::printf("melee\n");
    std::printf("  before\n");
    std::printf("    army\n");
    print_grp("      ", state, army);
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);
    UnitAddKey stay = UnitAddKey::None();
    UnitAddKey occ = UnitAddKey::None();
    const CityAssault r = CityAttackManager::melee(state, &army, cx, cy, &stay, &occ);
    const char* rs = (r == CityAssault::Ok) ? "Ok" : (r == CityAssault::Stall) ? "Stall" : "Fail";
    std::printf("  after result=%s\n", rs);
    if (stay.is_valid()) {
        std::printf("    stay\n");
        print_grp("      ", state, stay);
    }
    if (occ.is_valid()) {
        std::printf("    occupy\n");
        print_grp("      ", state, occ);
    }
    std::printf("    def\n");
    print_tile_seat("      ", state, cx, cy, def);
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    u32 want = 0;
    bool all = false;
    if (argc >= 2) {
        if (std::strcmp(argv[1], "all") == 0) {
            all = true;
        } else {
            want = static_cast<u32>(std::strtoul(argv[1], nullptr, 10));
            if (want == 0u) {
                std::printf("usage: city_attack_manager_tester [N|all]\n");
                return 1;
            }
        }
    }

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
    } else {
        std::printf("cache miss: running %u turns with %u players\n", G_TURN_CAP, G_PLAYERS);
        if (!run_warmup(state)) {
            std::printf("*** FAILED warmup/save\n");
            state.clear();
            return 1;
        }
    }
    sync_city_tile_owners(state);

    u32 ids[512];
    const u32 n = list_sieges(ids, 512u);
    if (n == 0u) {
        std::printf("no sieges in %s (run war_turn_handler_comp first)\n", G_SIEGE_DIR);
        state.clear();
        return 1;
    }
    if (!all && want == 0u) {
        std::printf("available sieges:");
        for (u32 i = 0; i < n; ++i) {
            std::printf(" %u", static_cast<unsigned>(ids[i]));
        }
        std::printf("\nusage: city_attack_manager_tester [N|all]\n");
        state.clear();
        return 0;
    }
    if (all) {
        for (u32 i = 0; i < n; ++i) {
            if (!replay_siege(state, ids[i])) {
                state.clear();
                return 1;
            }
        }
    } else {
        if (!replay_siege(state, want)) {
            state.clear();
            return 1;
        }
    }

    state.clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
