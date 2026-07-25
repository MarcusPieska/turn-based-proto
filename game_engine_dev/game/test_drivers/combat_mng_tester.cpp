//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "combat_mng.h"
#include "factory_game_array_simple.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "runtime_static_loader.h"
#include "runtime_statics.h"
#include "tile_attr_tables.h"
#include "unit_add_struct.h"
#include "unit_static_data.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Globals -
//================================================================================================================================

static RuntimeStaticLoader g_rt_loader;
static RuntimeStatics* g_rt_statics = nullptr;
static GameArraySimple g_map;
static int g_fails = 0;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool load_statics () {
    if (g_rt_statics != nullptr) {
        return true;
    }
    if (!g_rt_loader.load("../../data_io/runtime_static_loader_lib.so", "../../")) {
        return false;
    }
    g_rt_statics = &g_rt_loader.statics();
    return true;
}

static bool mk_map () {
    if (!Factory_GameArraySimple::init_test_grid(&g_map, 2u, 1u)) {
        return false;
    }
    GameTileSimple* a = g_map.tile(0u, 0u);
    GameTileSimple* b = g_map.tile(1u, 0u);
    a->m_clim = CLIMATE_NONE;
    b->m_clim = CLIMATE_NONE;
    a->m_road_typ = ROAD_NONE;
    b->m_road_typ = ROAD_NONE;
    a->m_terr = TERR_PLAINS[0];
    b->m_terr = TERR_PLAINS[0];
    a->m_ov = OV_NONE[0];
    b->m_ov = OV_NONE[0];
    a->m_riv = 0u;
    b->m_riv = 0u;
    return true;
}

static void set_scene (u8 terr, u8 ov, u8 riv) {
    GameTileSimple* a = g_map.tile(0u, 0u);
    GameTileSimple* b = g_map.tile(1u, 0u);
    a->m_terr = terr;
    b->m_terr = terr;
    a->m_ov = ov;
    b->m_ov = ov;
    a->m_riv = riv;
    b->m_riv = riv;
}

static UnitAddStruct mk_unit (u16 typ_idx, u16 x, u16 y) {
    UnitAddStruct u = {};
    u.m_x = x;
    u.m_y = y;
    u.m_unit_typ_idx = typ_idx;
    u.m_next_unit_on_tile = U16_KEY_NULL;
    u.m_next_unit_in_group = U16_KEY_NULL;
    u.m_mvt_points = 0;
    u.m_player_idx = 0u;
    u.m_health = UNIT_HEALTH;
    u.m_level = GREEN;
    u.m_misc = 0u;
    return u;
}

static u16 find_unit (cstr name) {
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 n = ud.get_item_count();
    for (u16 i = 0; i < n; ++i) {
        if (std::strcmp(ud.get_name(UnitStaticDataKey::from_raw(i)), name) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

struct DefBucket {
    u16 m_def;
    u16 m_typ;
};

static void eval_attack (const UnitAddStruct& atk_proto) {
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 n = ud.get_item_count();
    if (atk_proto.m_unit_typ_idx >= n) {
        std::printf("eval_attack: bad attacker typ\n");
        return;
    }
    cstr atk_name = ud.get_name(UnitStaticDataKey::from_raw(atk_proto.m_unit_typ_idx));
    std::printf("\n=== eval_attack: %s (atk=%u) vs unique defense buckets, GREEN on plains ===\n",
        atk_name,
        static_cast<u32>(ud.get_item(UnitStaticDataKey::from_raw(atk_proto.m_unit_typ_idx)).attack));
    DefBucket* bucks = new DefBucket[n];
    u16 buck_n = 0u;
    for (u16 i = 0; i < n; ++i) {
        const u16 def_val = ud.get_item(UnitStaticDataKey::from_raw(i)).defense;
        bool first = true;
        for (u16 j = 0; j < buck_n; ++j) {
            if (bucks[j].m_def == def_val) {
                first = false;
                break;
            }
        }
        if (!first) {
            continue;
        }
        bucks[buck_n].m_def = def_val;
        bucks[buck_n].m_typ = i;
        buck_n++;
    }
    for (u16 a = 0; a + 1u < buck_n; ++a) {
        for (u16 b = static_cast<u16>(a + 1u); b < buck_n; ++b) {
            if (bucks[b].m_def < bucks[a].m_def) {
                const DefBucket tmp = bucks[a];
                bucks[a] = bucks[b];
                bucks[b] = tmp;
            }
        }
    }
    for (u16 bi = 0; bi < buck_n; ++bi) {
        const u16 typ = bucks[bi].m_typ;
        const u16 def_val = bucks[bi].m_def;
        cstr def_name = ud.get_name(UnitStaticDataKey::from_raw(typ));
        UnitAddStruct atk = atk_proto;
        UnitAddStruct def = mk_unit(typ, 1u, 0u);
        const auto t0 = std::chrono::steady_clock::now();
        const u16 win_prob = CombatMng::resolve_win_prob(atk, def, g_map);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 ns = static_cast<f64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        const f64 pct = static_cast<f64>(win_prob) * 100.0 / 1000.0;
        std::printf("%-24s vs %-24s  def=%2u  win=%6.2f%%  time=%8.2f us\n",
            atk_name, def_name, static_cast<u32>(def_val), pct, ns / 1000.0);
    }
    delete[] bucks;
}

static void run_single_eval (cstr name) {
    const u16 typ = find_unit(name);
    if (typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", name);
        g_fails++;
        return;
    }
    set_scene(TERR_PLAINS[0], OV_NONE[0], 0u);
    eval_attack(mk_unit(typ, 0u, 0u));
}

struct TerrScene {
    cstr m_label;
    u8 m_terr;
    u8 m_ov;
    u8 m_riv;
};

static const TerrScene k_terr_scenes[] = {
    {"plains", TERR_PLAINS[0], OV_NONE[0], 0u},
    {"plains+forest", TERR_PLAINS[0], OV_FOREST[0], 0u},
    {"hills", TERR_HILLS[0], OV_NONE[0], 0u},
    {"hills+forest", TERR_HILLS[0], OV_FOREST[0], 0u},
    {"swamps", TERR_PLAINS[0], OV_SWAMP[0], 0u},
    {"hills+jungle", TERR_HILLS[0], OV_JUNGLE[0], 0u},
    {"hills+jungle+river", TERR_HILLS[0], OV_JUNGLE[0], 1u},
};

static void run_terrain_eval (cstr atk_name, cstr def_name) {
    const u16 atk_typ = find_unit(atk_name);
    const u16 def_typ = find_unit(def_name);
    if (atk_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", atk_name);
        g_fails++;
        return;
    }
    if (def_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", def_name);
        g_fails++;
        return;
    }
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 atk_str = ud.get_item(UnitStaticDataKey::from_raw(atk_typ)).attack;
    const u16 def_str = ud.get_item(UnitStaticDataKey::from_raw(def_typ)).defense;
    std::printf("\n=== terrain_eval: %s (atk=%u) vs %s (def=%u), GREEN ===\n",
        atk_name, static_cast<u32>(atk_str), def_name, static_cast<u32>(def_str));
    const u16 scene_n = static_cast<u16>(sizeof(k_terr_scenes) / sizeof(k_terr_scenes[0]));
    for (u16 i = 0; i < scene_n; ++i) {
        const TerrScene& sc = k_terr_scenes[i];
        set_scene(sc.m_terr, sc.m_ov, sc.m_riv);
        UnitAddStruct atk = mk_unit(atk_typ, 0u, 0u);
        UnitAddStruct def = mk_unit(def_typ, 1u, 0u);
        const auto t0 = std::chrono::steady_clock::now();
        const u16 win_prob = CombatMng::resolve_win_prob(atk, def, g_map);
        const auto t1 = std::chrono::steady_clock::now();
        const f64 ns = static_cast<f64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        const f64 pct = static_cast<f64>(win_prob) * 100.0 / 1000.0;
        std::printf("%-24s vs %-24s  %-20s  win=%6.2f%%  time=%8.2f us\n",
            atk_name, def_name, sc.m_label, pct, ns / 1000.0);
    }
}

static const u8 k_levels[] = {
    VERY_GREEN, GREEN, REGULAR, DISCIPLINED, HARDENED, VETERAN, COMMANDO, ELITE
};
static const cstr k_level_names[] = {
    "V.Green", "Green", "Regular", "Discipl", "Hardened", "Veteran", "Commando", "Elite"
};
static const u16 k_level_n = 8u;

static void run_training_eval (cstr atk_name, cstr def_name) {
    const u16 atk_typ = find_unit(atk_name);
    const u16 def_typ = find_unit(def_name);
    if (atk_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", atk_name);
        g_fails++;
        return;
    }
    if (def_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", def_name);
        g_fails++;
        return;
    }
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 atk_str = ud.get_item(UnitStaticDataKey::from_raw(atk_typ)).attack;
    const u16 def_str = ud.get_item(UnitStaticDataKey::from_raw(def_typ)).defense;
    set_scene(TERR_PLAINS[0], OV_NONE[0], 0u);
    std::printf("\n=== training_eval: %s (atk=%u) vs %s (def=%u), plains ===\n",
        atk_name, static_cast<u32>(atk_str), def_name, static_cast<u32>(def_str));
    std::printf("%-10s", "atk\\def");
    for (u16 d = 0; d < k_level_n; ++d) {
        std::printf(" %8s", k_level_names[d]);
    }
    std::printf("\n");
    for (u16 a = 0; a < k_level_n; ++a) {
        std::printf("%-10s", k_level_names[a]);
        for (u16 d = 0; d < k_level_n; ++d) {
            UnitAddStruct atk = mk_unit(atk_typ, 0u, 0u);
            UnitAddStruct def = mk_unit(def_typ, 1u, 0u);
            atk.m_level = k_levels[a];
            def.m_level = k_levels[d];
            const u16 win_prob = CombatMng::resolve_win_prob(atk, def, g_map);
            const f64 pct = static_cast<f64>(win_prob) * 100.0 / 1000.0;
            std::printf(" %7.2f%%", pct);
        }
        std::printf("\n");
    }
}

static const u8 k_healths[] = {10u, 20u, 30u, 40u, 50u, 60u, 70u, 80u, 90u, 100u};
static const u16 k_health_n = 10u;

static void run_health_eval (cstr atk_name, cstr def_name) {
    const u16 atk_typ = find_unit(atk_name);
    const u16 def_typ = find_unit(def_name);
    if (atk_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", atk_name);
        g_fails++;
        return;
    }
    if (def_typ == U16_KEY_NULL) {
        std::printf("FAILED find %s\n", def_name);
        g_fails++;
        return;
    }
    const UnitStaticData& ud = g_rt_statics->unit();
    const u16 atk_str = ud.get_item(UnitStaticDataKey::from_raw(atk_typ)).attack;
    const u16 def_str = ud.get_item(UnitStaticDataKey::from_raw(def_typ)).defense;
    set_scene(TERR_PLAINS[0], OV_NONE[0], 0u);
    std::printf("\n=== health_eval: %s (atk=%u) vs %s (def=%u), GREEN plains ===\n",
        atk_name, static_cast<u32>(atk_str), def_name, static_cast<u32>(def_str));
    std::printf("%-8s", "atk\\def");
    for (u16 d = 0; d < k_health_n; ++d) {
        std::printf(" %6u", static_cast<u32>(k_healths[d]));
    }
    std::printf("\n");
    for (u16 a = 0; a < k_health_n; ++a) {
        std::printf("%-8u", static_cast<u32>(k_healths[a]));
        for (u16 d = 0; d < k_health_n; ++d) {
            UnitAddStruct atk = mk_unit(atk_typ, 0u, 0u);
            UnitAddStruct def = mk_unit(def_typ, 1u, 0u);
            atk.m_health = k_healths[a];
            def.m_health = k_healths[d];
            const u16 win_prob = CombatMng::resolve_win_prob(atk, def, g_map);
            const f64 pct = static_cast<f64>(win_prob) * 100.0 / 1000.0;
            std::printf(" %5.1f%%", pct);
        }
        std::printf("\n");
    }
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    if (!load_statics()) {
        std::printf("FAILED load statics\n");
        return 1;
    }
    if (!TileAttrTables::setup(*g_rt_statics)) {
        std::printf("FAILED TileAttrTables::setup\n");
        return 1;
    }
    if (!CombatMng::setup(*g_rt_statics)) {
        std::printf("FAILED CombatMng::setup\n");
        return 1;
    }
    CombatMng::set_dials(85u, 25u, 1u);
    if (!mk_map()) {
        std::printf("FAILED map\n");
        return 1;
    }
    if (0) {
        run_single_eval("Archer");
        run_single_eval("Swordsman");
        run_single_eval("Knight");
        run_single_eval("Crusader");
        run_single_eval("Cavalry");
        run_single_eval("Tank");
        run_single_eval("Modern Armor");
    }
    if (0) {
        run_terrain_eval("Swordsman", "Spearman");
        run_terrain_eval("Swordsman", "Pikeman");
        run_terrain_eval("Swordsman", "Rifleman");
        run_terrain_eval("Swordsman", "Infantry");
        run_terrain_eval("Swordsman", "Mech Infantry");
    }
    if (0) {
        run_training_eval("Swordsman", "Spearman");
        run_training_eval("Swordsman", "Pikeman");
        run_training_eval("Swordsman", "Rifleman");
        run_training_eval("Swordsman", "Infantry");
        run_training_eval("Swordsman", "Mech Infantry");
    }
    if (1) {
        run_health_eval("Swordsman", "Spearman");
        run_health_eval("Swordsman", "Pikeman");
        run_health_eval("Swordsman", "Rifleman");
        run_health_eval("Swordsman", "Infantry");
        run_health_eval("Swordsman", "Mech Infantry");
    }
    CombatMng::clear();
    TileAttrTables::clear();
    return (g_fails > 0) ? 1 : 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
