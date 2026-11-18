//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <cerrno>
#include <chrono>
#include <new>
#include <sys/stat.h>
#include <sys/types.h>

#include "ai_proc_toggles.h"
#include "city.h"
#include "city_array.h"
#include "city_muster_grad.h"
#include "combat_mng.h"
#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "map_own_save_snap.h"
#include "mock_muster_siege.h"
#include "mvt_pts_boost_mng.h"
#include "player_ledger.h"
#include "prof_dbg_setup.h"
#include "runtime_static_loader.h"
#include "runtime_trace_dbg.h"
#include "tech_age_mng.h"
#include "tile_attr_tables.h"
#include "tile_entry_resolver.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_roster_mng.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_utility_helper.h"
#include "war_turn_handler.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//= Defaults =
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const char* G_DATA = "war-harness-save";
static const u32 G_TURN = 500u;
static const u32 G_RUN = 1000u;
static const char* G_COMP = "Spearman,Swordsman,Catapult";
static const u16 G_LUCKY_N[3] = {8u, 4u, 4u};
static const u16 G_OTHER_N[3] = {2u, 0u, 0u};
static const u16 G_LUCKY_CAP_N[3] = {40u, 20u, 20u};

//================================================================================================================================
//= Helpers =
//================================================================================================================================

static bool mkdir_p (cstr path) {
    return path != nullptr && (::mkdir(path, 0755) == 0 || errno == EEXIST);
}

static bool mkdir_deep (cstr path) {
    if (path == nullptr || path[0] == 0) {
        return false;
    }
    char tmp[512];
    if (std::snprintf(tmp, sizeof(tmp), "%s", path) <= 0) {
        return false;
    }
    const u32 n = static_cast<u32>(std::strlen(tmp));
    for (u32 i = 1u; i < n; ++i) {
        if (tmp[i] != '/') {
            continue;
        }
        tmp[i] = 0;
        if (!mkdir_p(tmp)) {
            return false;
        }
        tmp[i] = '/';
    }
    return mkdir_p(tmp);
}

static bool ieq (cstr a, cstr b) {
    if (a == nullptr || b == nullptr) {
        return false;
    }
    while (*a != 0 && *b != 0) {
        const int ca = std::tolower(static_cast<unsigned char>(*a++));
        const int cb = std::tolower(static_cast<unsigned char>(*b++));
        if (ca != cb) {
            return false;
        }
    }
    return *a == 0 && *b == 0;
}

static u16 unit_typ (const RuntimeStatics& st, cstr name) {
    const u16 n = st.unit().get_item_count();
    for (u16 i = 0u; i < n; ++i) {
        cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(i));
        if (ieq(nm, name)) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool parse_comp (cstr s, char out[3][64]) {
    if (s == nullptr) {
        return false;
    }
    char buf[256];
    if (std::snprintf(buf, sizeof(buf), "%s", s) <= 0) {
        return false;
    }
    u16 k = 0u;
    char* tok = std::strtok(buf, ",");
    while (tok != nullptr && k < 3u) {
        while (*tok == ' ' || *tok == '\t' || *tok == '[') {
            ++tok;
        }
        char* e = tok + std::strlen(tok);
        while (e > tok && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == ']')) {
            --e;
            *e = 0;
        }
        if (std::snprintf(out[k], 64, "%s", tok) <= 0) {
            return false;
        }
        ++k;
        tok = std::strtok(nullptr, ",");
    }
    return k == 3u;
}

static bool arg_val (cstr a, cstr key, cstr* out) {
    if (a == nullptr || key == nullptr || out == nullptr) {
        return false;
    }
    const u32 kn = static_cast<u32>(std::strlen(key));
    if (std::strncmp(a, key, kn) != 0 || a[kn] != '=') {
        return false;
    }
    *out = a + kn + 1u;
    return true;
}

static void set_target_unit_lvl (GameState& st, u8 lvl) {
    const u16 head = st.m_units.get_head_unit_add_idx();
    for (u16 k = 0u; k < head; ++k) {
        UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(k));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u16 seat = static_cast<u16>(u->m_player_idx);
        if (seat >= st.m_player_n || st.m_player_states[seat].m_lucky != 0u) {
            continue;
        }
        u->m_level = lvl;
    }
}

static void wipe_unit_hds (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0u; y < h; ++y) {
        for (u16 x = 0u; x < w; ++x) {
            (void)map.set_unit_hd(x, y, U16_KEY_NULL);
        }
    }
}

static void reset_units (GameState& st) {
    wipe_unit_hds(st.m_map);
    st.m_units.~UnitAddVector();
    new (&st.m_units) UnitAddVector();
}

static u16 city_n_owner (const CityArray& cities, u16 seat) {
    u16 n = 0u;
    const u16 cn = cities.get_city_count();
    for (u16 i = 0u; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c != nullptr && c->get_owner() == seat) {
            ++n;
        }
    }
    return n;
}

static u16 city_n_other (const CityArray& cities, u16 seat) {
    u16 n = 0u;
    const u16 cn = cities.get_city_count();
    for (u16 i = 0u; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c != nullptr && c->get_owner() != seat && c->get_owner() != U16_KEY_NULL) {
            ++n;
        }
    }
    return n;
}

static bool seat_cap_xy (const GameState& st, u16 seat, u16* ox, u16* oy) {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0u; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        *ox = c->get_x();
        *oy = c->get_y();
        return true;
    }
    return false;
}

static bool pick_near_enemy (const GameState& st, u16 seat, u16* out_enemy) {
    if (out_enemy == nullptr || st.m_player_states == nullptr) {
        return false;
    }
    u16 sx = 0u;
    u16 sy = 0u;
    if (!seat_cap_xy(st, seat, &sx, &sy)) {
        return false;
    }
    u32 best_d = 0xFFFFFFFFu;
    u16 best = U16_KEY_NULL;
    for (u16 e = 0u; e < st.m_player_n; ++e) {
        if (e == seat || st.m_player_states[e].m_is_active == 0u) {
            continue;
        }
        if (st.m_player_states[e].m_lucky != 0u) {
            continue;
        }
        u16 ex = 0u;
        u16 ey = 0u;
        if (!seat_cap_xy(st, e, &ex, &ey)) {
            continue;
        }
        const u32 adx = sx > ex ? static_cast<u32>(sx - ex) : static_cast<u32>(ex - sx);
        const u32 ady = sy > ey ? static_cast<u32>(sy - ey) : static_cast<u32>(ey - sy);
        const u32 d = adx + ady;
        if (d < best_d) {
            best_d = d;
            best = e;
        }
    }
    if (best == U16_KEY_NULL) {
        return false;
    }
    *out_enemy = best;
    return true;
}

static bool spawn_xy (
    GameState& st,
    u16 seat,
    u16 x,
    u16 y,
    const u16* typ,
    const u16* cnt,
    u16 typ_n,
    bool arty_first) {
    for (u16 i = 0u; i < typ_n; ++i) {
        const u16 t = arty_first ? static_cast<u16>(typ_n - 1u - i) : i;
        if (cnt[t] == 0u || typ[t] == U16_KEY_NULL) {
            continue;
        }
        for (u16 k = 0u; k < cnt[t]; ++k) {
            if (!st.spawn(x, y, seat, &typ[t], 1u)) {
                return false;
            }
        }
    }
    return true;
}

static bool spawn_seat (
    GameState& st,
    u16 seat,
    const u16* typ,
    const u16* cnt,
    u16 typ_n) {
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0u; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        if (!spawn_xy(st, seat, c->get_x(), c->get_y(), typ, cnt, typ_n, false)) {
            return false;
        }
    }
    return true;
}

static bool spawn_lucky_cap (GameState& st, u16 seat, const u16 typ[3]) {
    u16 x = 0u;
    u16 y = 0u;
    if (!seat_cap_xy(st, seat, &x, &y)) {
        return false;
    }
    return spawn_xy(st, seat, x, y, typ, G_LUCKY_CAP_N, 3u, true);
}

static bool seed_units (GameState& st, const u16 typ[3]) {
    reset_units(st);
    for (u16 seat = 0u; seat < st.m_player_n; ++seat) {
        if (st.m_player_states[seat].m_lucky == 0u) {
            continue;
        }
        if (!spawn_lucky_cap(st, seat, typ)) {
            return false;
        }
        if (!spawn_seat(st, seat, typ, G_LUCKY_N, 3u)) {
            return false;
        }
    }
    for (u16 seat = 0u; seat < st.m_player_n; ++seat) {
        if (st.m_player_states[seat].m_lucky != 0u) {
            continue;
        }
        if (st.m_player_states[seat].m_is_active == 0u) {
            continue;
        }
        if (!spawn_seat(st, seat, typ, G_OTHER_N, 3u)) {
            return false;
        }
    }
    return true;
}

static u16 find_lucky (const GameState& st) {
    for (u16 s = 0u; s < st.m_player_n; ++s) {
        if (st.m_player_states[s].m_lucky != 0u && st.m_player_states[s].m_is_active != 0u) {
            return s;
        }
    }
    return U16_KEY_NULL;
}

static u32 unit_n_seat (const GameState& st, u16 seat) {
    u32 n = 0u;
    const u16 head = st.m_units.get_head_unit_add_idx();
    for (u16 k = 0u; k < head; ++k) {
        const UnitAddStruct* u = st.m_units.get_unit_add(UnitAddKey::from_raw(k));
        if (u != nullptr && u->m_x != U16_KEY_NULL && static_cast<u16>(u->m_player_idx) == seat) {
            ++n;
        }
    }
    return n;
}

static void probe_mock_siege (GameState& st, u16 lucky, u16 enemy) {
    if (enemy == U16_KEY_NULL) {
        return;
    }
    MockMusterSiege mock;
    if (!mock.collect(st, lucky) || !mock.ok()) {
        std::printf("mock_probe collect fail army_n=%u\n", static_cast<unsigned>(mock.army_n()));
        return;
    }
    const u16 an0 = mock.army_n();
    const u16 br0 = mock.barrage_n(st);
    u16 tx = 0u;
    u16 ty = 0u;
    bool got = false;
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0u; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        tx = c->get_x();
        ty = c->get_y();
        got = true;
        break;
    }
    if (!got) {
        std::printf("mock_probe no enemy city army_n=%u barrage=%u\n",
            static_cast<unsigned>(an0), static_cast<unsigned>(br0));
        return;
    }
    MockSiegeRslt r = {};
    const bool ok = mock.siege(st, tx, ty, &r);
    std::printf(
        "mock_probe city=(%u,%u) collect_n=%u barrage=%u ok=%u taken=%u army0=%u live=%u def0=%u def_live=%u after_n=%u\n",
        static_cast<unsigned>(tx), static_cast<unsigned>(ty),
        static_cast<unsigned>(an0), static_cast<unsigned>(br0),
        ok ? 1u : 0u, r.m_taken ? 1u : 0u,
        static_cast<unsigned>(r.m_army_n0), static_cast<unsigned>(r.m_army_live),
        static_cast<unsigned>(r.m_def_n0), static_cast<unsigned>(r.m_def_live),
        static_cast<unsigned>(mock.army_n()));
}

static u16 try_engage (GameState& st, u16 lucky) {
    if (lucky >= st.m_player_n || WarTurnHandler::is_engaged(lucky)) {
        return U16_KEY_NULL;
    }
    if (st.m_player_states[lucky].m_at_war != 0u) {
        st.m_player_states[lucky].m_at_war = 0u;
    }
    u16 enemy = U16_KEY_NULL;
    if (!WarTurnHandler::pick_enemy(st, lucky, &enemy)) {
        if (!pick_near_enemy(st, lucky, &enemy)) {
            return U16_KEY_NULL;
        }
    }
    if (WarTurnHandler::engage(st, lucky, enemy)) {
        st.m_player_states[lucky].m_at_war = 1u;
        return enemy;
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//= Main =
//================================================================================================================================

int main (int argc, char** argv) {
    std::printf(
        "./war_harness_save_comp [turn=%u] [run_n=%u] [composition=%s] [unit_lvl=0]\n",
        G_TURN, G_RUN, G_COMP);
#if defined(LUCKY_SEAT_INSTANT_COMBAT_WIN)
    std::printf("instant_win=ON\n");
#else
    std::printf("instant_win=OFF\n");
#endif
    u32 turn0 = G_TURN;
    u32 run_n = G_RUN;
    u32 unit_lvl = 0u;
    char comp_s[256];
    std::snprintf(comp_s, sizeof(comp_s), "%s", G_COMP);
    for (int i = 1; i < argc; ++i) {
        cstr v = nullptr;
        if (arg_val(argv[i], "turn", &v)) {
            turn0 = static_cast<u32>(std::strtoul(v, nullptr, 10));
            continue;
        }
        if (arg_val(argv[i], "run_n", &v)) {
            run_n = static_cast<u32>(std::strtoul(v, nullptr, 10));
            continue;
        }
        if (arg_val(argv[i], "composition", &v)) {
            std::snprintf(comp_s, sizeof(comp_s), "%s", v);
            continue;
        }
        if (arg_val(argv[i], "unit_lvl", &v)) {
            unit_lvl = static_cast<u32>(std::strtoul(v, nullptr, 10));
            continue;
        }
        std::printf("war_harness_save: bad arg '%s' (want name=value)\n", argv[i]);
        return 1;
    }
    std::printf(
        "spawn lucky=%u/%u/%u other=%u/%u/%u per city; lucky_cap=%u/%u/%u; unit_lvl=%u (targets)\n",
        static_cast<unsigned>(G_LUCKY_N[0]), static_cast<unsigned>(G_LUCKY_N[1]),
        static_cast<unsigned>(G_LUCKY_N[2]), static_cast<unsigned>(G_OTHER_N[0]),
        static_cast<unsigned>(G_OTHER_N[1]), static_cast<unsigned>(G_OTHER_N[2]),
        static_cast<unsigned>(G_LUCKY_CAP_N[0]), static_cast<unsigned>(G_LUCKY_CAP_N[1]),
        static_cast<unsigned>(G_LUCKY_CAP_N[2]), static_cast<unsigned>(unit_lvl));
    char names[3][64];
    if (!parse_comp(comp_s, names)) {
        std::printf("war_harness_save: bad composition '%s' (want a,b,c)\n", comp_s);
        return 1;
    }
    EvalPaths paths;
    if (!paths.load(G_PATHS) || !paths.ok()) {
        std::printf("war_harness_save: EvalPaths load failed\n");
        return 1;
    }
    char map_p[512];
    char cities_p[512];
    char players_p[512];
    if (!paths.map_path(turn0, map_p, sizeof(map_p))
        || !paths.cities_path(turn0, cities_p, sizeof(cities_p))
        || !paths.players_path(turn0, players_p, sizeof(players_p))) {
        std::printf("war_harness_save: path build failed\n");
        return 1;
    }
    {
        std::FILE* fp = std::fopen(map_p, "rb");
        if (fp == nullptr) {
            std::printf("war_harness_save: missing save turn=%u (%s)\n", turn0, map_p);
            return 1;
        }
        std::fclose(fp);
    }
    char eval_dir[512];
    char data_dir[512];
    char out_dir[512];
    if (!paths.eval_dir(eval_dir, sizeof(eval_dir))
        || !paths.data_dir(G_DATA, data_dir, sizeof(data_dir))) {
        std::printf("war_harness_save: out path failed\n");
        return 1;
    }
    if (std::snprintf(out_dir, sizeof(out_dir), "%s/%s", eval_dir, G_DATA) <= 0
        || !mkdir_deep(out_dir) || !mkdir_deep(data_dir)) {
        std::printf("war_harness_save: mkdir failed\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("war_harness_save: runtime statics failed\n");
        return 1;
    }
    const RuntimeStatics& rst = loader.statics();
    if (!TechAgeMng::setup(rst)) {
        std::printf("war_harness_save: TechAgeMng::setup failed\n");
        return 1;
    }
    u16 typ[3] = {U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL};
    for (u16 i = 0u; i < 3u; ++i) {
        typ[i] = unit_typ(rst, names[i]);
        if (typ[i] == U16_KEY_NULL) {
            std::printf("war_harness_save: unknown unit '%s'\n", names[i]);
            TechAgeMng::clear();
            return 1;
        }
    }
    GameState st;
    st.m_statics = &rst;
    if (!GameIo::load_map_tiles(map_p, st.m_map)
        || !GameIo::load_cities(cities_p, st.m_cities)
        || !GameIo::load_players(players_p, st)) {
        std::printf("war_harness_save: load save failed turn=%u\n", turn0);
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    st.m_current_turn = turn0;
    st.m_turn_limit = turn0 + run_n;
    st.m_players_remaining = st.m_player_n;
    if (!st.m_combat_mods.setup(rst)) {
        std::printf("war_harness_save: CombatMods::setup failed\n");
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    if (!seed_units(st, typ)) {
        std::printf("war_harness_save: seed_units failed\n");
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    {
        const u16 lvl_n = rst.unit_level().get_item_count();
        u8 lvl = static_cast<u8>(unit_lvl);
        if (lvl_n > 0u && unit_lvl >= static_cast<u32>(lvl_n)) {
            lvl = static_cast<u8>(lvl_n - 1u);
        }
        set_target_unit_lvl(st, lvl);
    }
    const u16 lucky = find_lucky(st);
    if (lucky == U16_KEY_NULL) {
        std::printf("war_harness_save: no lucky seat\n");
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    for (u16 s = 0u; s < st.m_player_n; ++s) {
        st.m_player_states[s].m_at_war = 0u;
        if (s != lucky) {
            st.m_player_states[s].m_ai_controlled = 0u;
        } else {
            st.m_player_states[s].m_ai_controlled = 1u;
        }
    }
    const u16 w = st.m_map.width();
    const u16 h = st.m_map.height();
    WhiteboardMng::init(w, h);
    if (!TileAttrTables::ready() && !TileAttrTables::setup(rst)) {
        std::printf("war_harness_save: TileAttrTables failed\n");
        WhiteboardMng::terminate();
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    if (!UnitMovementMng::setup_mvt_costs(rst)
        || !MvtPtsBoostMng::setup(st.m_player_n)
        || !CombatMng::setup(rst)
        || !UnitRosterMng::setup(rst)
        || !UnitUtilityHelper::setup(rst)) {
        std::printf("war_harness_save: combat/mvt setup failed\n");
        CombatMng::clear();
        UnitRosterMng::clear();
        UnitUtilityHelper::clear();
        WhiteboardMng::terminate();
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    UnitMovementMng::bind_state(&st);
    TileEntryResolver::bind_state(&st);
    PlayerLedger::bind_state(&st);
    City::bind_player_states(st.m_player_states, st.m_player_n);
    MvtPtsBoostMng::refresh(st);
    ProfDbgSetup::setup(out_dir);
    if (!WarTurnHandler::begin(st)) {
        std::printf("war_harness_save: WarTurnHandler::begin failed\n");
        UnitUtilityHelper::clear();
        UnitRosterMng::clear();
        CombatMng::clear();
        WhiteboardMng::terminate();
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    if (!CityMusterGrad::begin(st)) {
        std::printf("war_harness_save: CityMusterGrad::begin failed\n");
        WarTurnHandler::clear();
        UnitUtilityHelper::clear();
        UnitRosterMng::clear();
        CombatMng::clear();
        WhiteboardMng::terminate();
        st.clear();
        TechAgeMng::clear();
        return 1;
    }
    const u16 c0 = city_n_owner(st.m_cities, lucky);
    const u16 o0 = city_n_other(st.m_cities, lucky);
    const u32 u0 = unit_n_seat(st, lucky);
    char img0[768];
    char img1[768];
    char trace_p[768];
    std::snprintf(img0, sizeof(img0), "%s/own_t%04u_start.ppm", out_dir, turn0);
    std::snprintf(img1, sizeof(img1), "%s/own_t%04u_end.ppm", out_dir, turn0 + run_n);
    std::snprintf(trace_p, sizeof(trace_p), "%s/war_harness.trace", out_dir);
    if (!MapOwnSaveSnap::write(img0, st.m_map)) {
        std::printf("war_harness_save: write start map failed\n");
    }
    TRACE_SETUP((trace_p));
    u16 cur_enemy = try_engage(st, lucky);
    probe_mock_siege(st, lucky, cur_enemy);
    u32 engage_n = (cur_enemy != U16_KEY_NULL) ? 1u : 0u;
    u16 last_cities = c0;
    u16 peak_cities = c0;
    u32 gain_events = 0u;
    u32 disengage_n = 0u;
    bool was_eng = WarTurnHandler::is_engaged(lucky);
    if (cur_enemy != U16_KEY_NULL) {
        std::printf(
            "engage turn=%u enemy=%u enemy_cities=%u lucky_cities=%u units=%u\n",
            turn0, static_cast<unsigned>(cur_enemy),
            static_cast<unsigned>(city_n_owner(st.m_cities, cur_enemy)),
            static_cast<unsigned>(c0), static_cast<unsigned>(u0));
    }
    const auto t0 = std::chrono::steady_clock::now();
    for (u32 i = 0u; i < run_n; ++i) {
        st.m_current_turn = st.m_current_turn + 1u;
        MvtPtsBoostMng::refresh(st);
        const bool eng0 = WarTurnHandler::is_engaged(lucky);
        const u16 new_e = try_engage(st, lucky);
        if (new_e != U16_KEY_NULL) {
            cur_enemy = new_e;
            ++engage_n;
            std::printf(
                "\nengage turn=%u enemy=%u enemy_cities=%u lucky_cities=%u units=%u\n",
                st.m_current_turn, static_cast<unsigned>(cur_enemy),
                static_cast<unsigned>(city_n_owner(st.m_cities, cur_enemy)),
                static_cast<unsigned>(city_n_owner(st.m_cities, lucky)),
                static_cast<unsigned>(unit_n_seat(st, lucky)));
        }
        CityMusterGrad::handle(st);
        WarTurnHandler::handle(st);
        const bool eng1 = WarTurnHandler::is_engaged(lucky);
        if (eng0 && !eng1) {
            ++disengage_n;
            std::printf(
                "\ndisengage turn=%u enemy=%u lucky_cities=%u units=%u\n",
                st.m_current_turn,
                static_cast<unsigned>(cur_enemy),
                static_cast<unsigned>(city_n_owner(st.m_cities, lucky)),
                static_cast<unsigned>(unit_n_seat(st, lucky)));
            cur_enemy = U16_KEY_NULL;
        }
        const u16 cn = city_n_owner(st.m_cities, lucky);
        if (cn > last_cities) {
            ++gain_events;
            last_cities = cn;
            if (cn > peak_cities) {
                peak_cities = cn;
            }
        } else if (cn < last_cities) {
            last_cities = cn;
        }
        was_eng = eng1;
        std::printf(
            "\rturn %u/%u cities=%u units=%u eng=%u enemy=%u  ",
            st.m_current_turn, turn0 + run_n,
            static_cast<unsigned>(cn),
            static_cast<unsigned>(unit_n_seat(st, lucky)),
            eng1 ? 1u : 0u,
            static_cast<unsigned>(cur_enemy));
        std::fflush(stdout);
        (void)was_eng;
        (void)eng0;
    }
    std::printf("\n");
    const auto t1 = std::chrono::steady_clock::now();
    const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    if (!MapOwnSaveSnap::write(img1, st.m_map)) {
        std::printf("war_harness_save: write end map failed\n");
    }
    const u16 c1 = city_n_owner(st.m_cities, lucky);
    const u32 u1 = unit_n_seat(st, lucky);
    const u16 gained = (c1 > c0) ? static_cast<u16>(c1 - c0) : 0u;
    const double pct = (o0 == 0u) ? 0.0 : (100.0 * static_cast<double>(gained) / static_cast<double>(o0));
    std::printf(
        "war_harness_save turn0=%u run=%u lucky=%u units=%u,%u,%u\n",
        turn0, run_n, static_cast<unsigned>(lucky),
        static_cast<unsigned>(typ[0]), static_cast<unsigned>(typ[1]), static_cast<unsigned>(typ[2]));
    std::printf(
        "cities lucky %u -> %u (gained=%u peak=%u) other0=%u conquest_pct=%.2f loop_ms=%.1f\n",
        static_cast<unsigned>(c0), static_cast<unsigned>(c1), static_cast<unsigned>(gained),
        static_cast<unsigned>(peak_cities), static_cast<unsigned>(o0), pct, ms);
    std::printf(
        "units %u -> %u engages=%u disengages=%u capture_events=%u\n",
        static_cast<unsigned>(u0), static_cast<unsigned>(u1),
        static_cast<unsigned>(engage_n), static_cast<unsigned>(disengage_n),
        static_cast<unsigned>(gain_events));
    std::printf("maps: %s\n      %s\ntrace: %s\n", img0, img1, trace_p);
    CityMusterGrad::clear();
    WarTurnHandler::clear();
    UnitUtilityHelper::clear();
    UnitRosterMng::clear();
    CombatMng::clear();
    MvtPtsBoostMng::clear();
    WhiteboardMng::terminate();
    st.clear();
    TechAgeMng::clear();
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
