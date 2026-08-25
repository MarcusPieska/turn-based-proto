//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>
#include <vector>

#include "bit_array.h"
#include "build_adds_array.h"
#include "city.h"
#include "city_border.h"
#include "city_tracer.h"
#include "city_turn_handler.h"
#include "defensive_unit_turn_handler.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "map_overlay_enum.h"
#include "map_overlay_static_key.h"
#include "tile_imp_helper.h"
#include "worker_job_imp_index.h"
#include "research_turn_handler.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "std_add_helper.h"
#include "tile_usage.h"
#include "tile_work_assessor.h"
#include "tile_yields.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_type_static_key.h"
#include "whiteboard_mng.h"
#include "worker_helper.h"
#include "worker_job_static_key.h"
#include "worker_job_imp_enum.h"
#include "worker_job_imp_static_key.h"
#include "worker_build_progress.h"
#include "worker_guidance.h"
#include "worker_turn_handler.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_GRN = "\033[32m";
static const char* G_RED = "\033[31m";
static const char* G_RST = "\033[0m";

static void note_assert (bool ok, cstr msg) {
    std::printf("%s  %s: %s%s\n", ok ? G_GRN : G_RED, ok ? "PASS" : "FAIL", msg, G_RST);
}

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/worker-turn-mng";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/worker-turn-mng/game_loop.trace";
static const char* G_CITY_LOG = "/home/w/Projects/simple-map-gen/worker-turn-mng/cities.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100;
static const u32 G_TURN_CAP = 300u;
static const u32 G_TURN_CAP_EXT = 1000u;
static const u16 G_CLAIM_CULT = 25u;
static u32 g_ppm_every = 10u;
static bool g_time_only = false;
static bool g_gradual_tech = false;
static u32 g_tech_iv = 0;
static u32 g_tech_cur = 0;
static char g_terr[320];
static char g_clim[320];
static char g_riv[320];
static char g_ov[320];
static char g_res[320];

static const u8 k_own_pal[][3] = {
    {220, 40, 40},
    {40, 90, 220},
    {40, 170, 70},
    {220, 110, 30},
    {190, 40, 170},
    {30, 170, 170},
    {150, 70, 30},
    {100, 40, 180},
};
static const u16 k_own_pal_n = static_cast<u16>(sizeof(k_own_pal) / sizeof(k_own_pal[0]));

//================================================================================================================================
//=> - Hot-path timers / job tallies -
//================================================================================================================================

struct HotTimer {
    const char* name;
    u64 n;
    u64 ns;
};

static HotTimer g_tm_refresh = {"SettlerTurnHandler::refresh_targets", 0, 0};
static HotTimer g_tm_city = {"CityTurnHandler::handle", 0, 0};
static HotTimer g_tm_research = {"ResearchTurnHandler::handle", 0, 0};
static HotTimer g_tm_settler = {"SettlerTurnHandler::handle", 0, 0};
static HotTimer g_tm_worker = {"WorkerTurnHandler::handle", 0, 0};
static HotTimer g_tm_defense = {"DefensiveUnitTurnHandler::handle", 0, 0};
static HotTimer g_tm_spawn = {"spawn_city_workers (tester)", 0, 0};
static HotTimer g_tm_turn = {"turn e2e (city+unit)", 0, 0};

static void tm_add (HotTimer* t, u64 ns) {
    t->n = t->n + 1u;
    t->ns = t->ns + ns;
}

static void tm_report (const HotTimer& t) {
    const double total_ms = static_cast<double>(t.ns) / 1.0e6;
    const double avg_us = (t.n == 0) ? 0.0 : (static_cast<double>(t.ns) / static_cast<double>(t.n)) / 1.0e3;
    std::printf("  %-36s  calls=%llu  total=%.3f ms  avg=%.3f us\n",
        t.name,
        static_cast<unsigned long long>(t.n),
        total_ms,
        avg_us);
}

static void tm_report_all () {
    const HotTimer* rows[] = {
        &g_tm_refresh, &g_tm_city, &g_tm_research, &g_tm_settler, &g_tm_worker, &g_tm_defense
    };
    u64 sum_ns = 0;
    u64 sum_n = 0;
    for (u32 i = 0; i < sizeof(rows) / sizeof(rows[0]); ++i) {
        tm_report(*rows[i]);
        sum_ns = sum_ns + rows[i]->ns;
        sum_n = sum_n + rows[i]->n;
    }
    const double total_ms = static_cast<double>(sum_ns) / 1.0e6;
    const double avg_us = (sum_n == 0) ? 0.0 : (static_cast<double>(sum_ns) / static_cast<double>(sum_n)) / 1.0e3;
    std::printf("  %-36s  calls=%llu  total=%.3f ms  avg=%.3f us\n",
        "handlers sum",
        static_cast<unsigned long long>(sum_n),
        total_ms,
        avg_us);
    tm_report(g_tm_spawn);
    tm_report(g_tm_turn);
}

struct JobTot {
    u16 m_job;
    u16 m_imp;
    u32 m_n;
};

static std::vector<JobTot> g_job_tot;
static const RuntimeStatics* g_st = nullptr;
static GameState* g_state = nullptr;
static u16 g_cur_unit_idx = U16_KEY_NULL;
static u32 g_job_apps = 0;
static u32 g_tile_skip_viol = 0;

struct UnitTileTrack {
    u16 m_x; // Last job tile x; U16_KEY_NULL if unset
    u16 m_y; // Last job tile y; U16_KEY_NULL if unset
    u8 m_on; // 1 when this worker has applied a job on m_x/m_y
};

static std::vector<UnitTileTrack> g_unit_tile;

static void unit_tile_ensure (u16 unit_idx) {
    if (unit_idx < g_unit_tile.size()) {
        return;
    }
    g_unit_tile.resize(static_cast<size_t>(unit_idx) + 1u);
}

static u16 unit_home_city (const GameState& state, const UnitAddStruct* unit) {
    const u16 idx = WorkerHelper::get_data(unit);
    const City* c = state.m_cities.get_city(idx);
    if (c != nullptr && c->get_owner() == unit->m_player_idx) {
        return idx;
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* city = state.m_cities.get_city(i);
        if (city == nullptr || city->get_owner() != unit->m_player_idx) {
            continue;
        }
        if (city->get_x() == unit->m_x && city->get_y() == unit->m_y) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static bool tile_pending_work (const GameState& state, u16 ux, u16 uy) {
    if (g_state == nullptr || state.m_map.get_planned_city(ux, uy) != 0u) {
        return false;
    }
    const TileAssignIntent intent = static_cast<TileAssignIntent>(state.m_map.get_tile_usage(ux, uy));
    u16 job = U16_KEY_NULL;
    u16 imp = U16_KEY_NULL;
    return WorkerGuidance::next_work(ux, uy, intent, &job, &imp);
}

static u32 job_tot_n (u16 job, u16 imp) {
    for (size_t i = 0; i < g_job_tot.size(); ++i) {
        if (g_job_tot[i].m_job == job && g_job_tot[i].m_imp == imp) {
            return g_job_tot[i].m_n;
        }
    }
    return 0u;
}

static void on_job (u16 x, u16 y, u16 job, u16 imp, u8 intent) {
    (void)intent;
    if (g_st == nullptr || g_state == nullptr || g_cur_unit_idx == U16_KEY_NULL) {
        return;
    }
    const UnitAddStruct* unit = g_state->m_units.get_unit_add(UnitAddKey::from_raw(g_cur_unit_idx));
    if (unit == nullptr || unit_home_city(*g_state, unit) == U16_KEY_NULL) {
        return;
    }
    unit_tile_ensure(g_cur_unit_idx);
    UnitTileTrack& tr = g_unit_tile[g_cur_unit_idx];
    if (tr.m_on != 0u && (tr.m_x != x || tr.m_y != y)) {
        if (tile_pending_work(*g_state, tr.m_x, tr.m_y)) {
            g_tile_skip_viol = g_tile_skip_viol + 1u;
        }
    }
    tr.m_x = x;
    tr.m_y = y;
    tr.m_on = 1u;
    g_job_apps = g_job_apps + 1u;
    for (size_t i = 0; i < g_job_tot.size(); ++i) {
        if (g_job_tot[i].m_job == job && g_job_tot[i].m_imp == imp) {
            g_job_tot[i].m_n = g_job_tot[i].m_n + 1u;
            return;
        }
    }
    JobTot t;
    t.m_job = job;
    t.m_imp = imp;
    t.m_n = 1u;
    g_job_tot.push_back(t);
}

static void print_job_tots () {
    std::printf(" totals by job:\n");
    if (g_st == nullptr) {
        return;
    }
    std::vector<u16> jobs;
    for (size_t i = 0; i < g_job_tot.size(); ++i) {
        const u16 j = g_job_tot[i].m_job;
        bool seen = false;
        for (size_t k = 0; k < jobs.size(); ++k) {
            if (jobs[k] == j) {
                seen = true;
                break;
            }
        }
        if (!seen) {
            jobs.push_back(j);
        }
    }
    std::sort(jobs.begin(), jobs.end());
    for (size_t ji = 0; ji < jobs.size(); ++ji) {
        const u16 job = jobs[ji];
        cstr jnm = g_st->worker_job().get_name(WorkerJobStaticDataKey::from_raw(job));
        if (jnm == nullptr) {
            jnm = "unknown_job";
        }
        std::printf("  %s: %u\n", jnm, (unsigned)job_tot_n(job, U16_KEY_NULL));
        std::vector<u16> imps;
        for (size_t i = 0; i < g_job_tot.size(); ++i) {
            if (g_job_tot[i].m_job != job || g_job_tot[i].m_imp == U16_KEY_NULL) {
                continue;
            }
            imps.push_back(g_job_tot[i].m_imp);
        }
        std::sort(imps.begin(), imps.end());
        for (size_t ii = 0; ii < imps.size(); ++ii) {
            cstr inm = g_st->worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(imps[ii]));
            if (inm == nullptr) {
                inm = "unknown_imp";
            }
            std::printf("    %s: %u\n", inm, (unsigned)job_tot_n(job, imps[ii]));
        }
    }
}

//================================================================================================================================
//=> - Setup helpers -
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

static bool is_worker_typ (const RuntimeStatics& st, u16 typ_idx) {
    const UnitStaticDataKey uk = UnitStaticDataKey::from_raw(typ_idx);
    const u16 ut = st.unit().get_item(uk).type;
    const UnitTypeStaticDataKey tk = UnitTypeStaticDataKey::from_raw(ut);
    return std::strcmp(st.unit_type().get_name(tk), "LAND_WORKER") == 0;
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

static u16 count_cities (const GameState& state) {
    u16 n = 0;
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c != nullptr && c->get_owner() != U16_KEY_NULL) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static u16 count_workers (const GameState& state) {
    if (state.m_statics == nullptr) {
        return 0;
    }
    u16 n = 0;
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (is_worker_typ(*state.m_statics, u->m_unit_typ_idx)) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static const u16 G_WORKERS_PER_CITY = 2u;
static u16 g_city_spawned_n = 0;

static u16 spawn_workers_for_city (GameState& state, u16 city_idx, u16 worker_typ, u16 n) {
    City* city = state.m_cities.get_city(city_idx);
    if (city == nullptr || city->get_owner() == U16_KEY_NULL) {
        return 0;
    }
    u16 spawned = 0;
    for (u16 k = 0; k < n; ++k) {
        UnitAddKey key = UnitAddKey::None();
        if (!UnitMovementMng::place_on_tile(state, city->get_x(), city->get_y(), city->get_owner(), worker_typ, &key)) {
            break;
        }
        UnitAddStruct* u = state.m_units.get_unit_add(key);
        if (u == nullptr) {
            break;
        }
        WorkerHelper::set_data(u, city_idx);
        spawned = static_cast<u16>(spawned + 1u);
    }
    return spawned;
}

static u16 spawn_workers_for_new_cities (GameState& state, u16 worker_typ) {
    const u16 cn = state.m_cities.get_city_count();
    u16 spawned = 0;
    for (u16 i = g_city_spawned_n; i < cn; ++i) {
        spawned = static_cast<u16>(spawned + spawn_workers_for_city(state, i, worker_typ, G_WORKERS_PER_CITY));
    }
    g_city_spawned_n = cn;
    return spawned;
}

static void unlock_all_tech (GameState& state) {
    const u16 tech_n = state.m_statics->tech().get_item_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            ps.m_techs_researched = new BitArrayCL(tech_n);
        }
        if (ps.m_techs_researched == nullptr) {
            continue;
        }
        for (u32 i = 0; i < ps.m_techs_researched->get_count(); ++i) {
            ps.m_techs_researched->set_bit(i);
        }
    }
}

static void ensure_tech_arrays (GameState& state) {
    const u16 tech_n = state.m_statics->tech().get_item_count();
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            ps.m_techs_researched = new BitArrayCL(tech_n);
        }
    }
}

static void unlock_tech_ix (GameState& state, BitArrayCL& tech, u32 ix) {
    if (ix >= tech.get_count()) {
        return;
    }
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        if (ps.m_techs_researched == nullptr) {
            continue;
        }
        ps.m_techs_researched->set_bit(ix);
        ps.m_tech_just_researched = 1;
    }
    tech.set_bit(ix);
    for (u16 p = 0; p < state.m_player_n; ++p) {
        City::refresh_city_worker_flags(state, p);
    }
}

static void gradual_tech_turn (GameState& state, BitArrayCL& tech, u32 turn) {
    if (!g_gradual_tech || g_tech_iv == 0u) {
        return;
    }
    if (turn == 0u || (turn % g_tech_iv) != 0u) {
        return;
    }
    if (g_tech_cur >= tech.get_count()) {
        return;
    }
    unlock_tech_ix(state, tech, g_tech_cur);
    g_tech_cur = g_tech_cur + 1u;
}

static void claim_city_borders (GameState& state) {
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        CityBorder::claim_expand(c->get_x(), c->get_y(), 0, G_CLAIM_CULT, static_cast<u8>(c->get_owner()));
    }
}

static void refill_mp (GameState& state, u16 unit_idx) {
    UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
    if (u == nullptr || state.m_statics == nullptr) {
        return;
    }
    const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
    if (ut == state.m_land_worker_type_idx) {
        WorkerBuildProgress::refill_mp(state, unit_idx);
        return;
    }
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * state.m_statics->config().get_mov_pt_per_turn());
}

static void after_city_turns (GameState& state) {
    for (u16 p = 0; p < state.m_player_n; ++p) {
        if (!g_gradual_tech) {
            const auto t0 = std::chrono::steady_clock::now();
            ResearchTurnHandler::handle(state, p);
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_research, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
            City::refresh_city_worker_flags(state, p);
        }
        PlayerState& ps = state.m_player_states[p];
        ps.m_last_turn_population_count = ps.m_this_turn_population_count;
        ps.m_last_turn_city_count = ps.m_this_turn_city_count;
        ps.m_this_turn_population_count = 0;
        ps.m_this_turn_city_count = 0;
        ps.m_last_turn_settler_count = 0;
        ps.m_last_turn_worker_count = 0;
        ps.m_defensive_unit_count = 0;
    }
}

static void run_city_turns (GameState& state) {
    {
        const auto t0 = std::chrono::steady_clock::now();
        SettlerTurnHandler::refresh_targets(state);
        const auto t1 = std::chrono::steady_clock::now();
        tm_add(&g_tm_refresh, static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        if (state.m_cities.get_city(i) == nullptr) {
            continue;
        }
        const auto t0 = std::chrono::steady_clock::now();
        CityTurnHandler::handle(state, i);
        const auto t1 = std::chrono::steady_clock::now();
        tm_add(&g_tm_city, static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
    }
    LOG_CITY_FLUSH(());
    after_city_turns(state);
}

static void run_unit_turns (GameState& state) {
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const u16 unit_idx = static_cast<u16>(idx);
        UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(unit_idx));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        const u16 ut = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (ut == state.m_land_settler_type_idx) {
            refill_mp(state, unit_idx);
            const auto t0 = std::chrono::steady_clock::now();
            SettlerTurnHandler::handle(state, unit_idx);
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_settler, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        } else if (ut == state.m_land_worker_type_idx) {
            refill_mp(state, unit_idx);
            g_cur_unit_idx = unit_idx;
            const auto t0 = std::chrono::steady_clock::now();
            WorkerTurnHandler::handle(state, unit_idx);
            g_cur_unit_idx = U16_KEY_NULL;
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_worker, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        } else if (ut == state.m_land_defense_type_idx) {
            const auto t0 = std::chrono::steady_clock::now();
            DefensiveUnitTurnHandler::handle(state, unit_idx);
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_defense, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }
    }
}

//================================================================================================================================
//=> - PPM visualization -
//================================================================================================================================

static void set_px (u8* rgb, u16 w, u16 h, u16 x, u16 y, u8 r, u8 g, u8 b) {
    if (x >= w || y >= h) {
        return;
    }
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    rgb[i + 0] = r;
    rgb[i + 1] = g;
    rgb[i + 2] = b;
}

static void shade_own (u8* rgb, u16 w, u16 h, u16 x, u16 y, u16 seat) {
    if (x >= w || y >= h) {
        return;
    }
    const u8* c = k_own_pal[seat % k_own_pal_n];
    const u32 i = (static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x)) * 3u;
    const u16 cr = (static_cast<u16>(c[0]) * 5u) / 8u;
    const u16 cg = (static_cast<u16>(c[1]) * 5u) / 8u;
    const u16 cb = (static_cast<u16>(c[2]) * 5u) / 8u;
    rgb[i + 0] = static_cast<u8>((static_cast<u16>(rgb[i + 0]) + cr * 3u) / 4u);
    rgb[i + 1] = static_cast<u8>((static_cast<u16>(rgb[i + 1]) + cg * 3u) / 4u);
    rgb[i + 2] = static_cast<u8>((static_cast<u16>(rgb[i + 2]) + cb * 3u) / 4u);
}

static void paint_black_mark (u8* rgb, u16 w, u16 h, u16 x, u16 y) {
    set_px(rgb, w, h, x, y, 0, 0, 0);
    if (x > 0) {
        set_px(rgb, w, h, static_cast<u16>(x - 1u), y, 0, 0, 0);
    }
    if (static_cast<u32>(x) + 1u < static_cast<u32>(w)) {
        set_px(rgb, w, h, static_cast<u16>(x + 1u), y, 0, 0, 0);
    }
    if (y > 0) {
        set_px(rgb, w, h, x, static_cast<u16>(y - 1u), 0, 0, 0);
    }
    if (static_cast<u32>(y) + 1u < static_cast<u32>(h)) {
        set_px(rgb, w, h, x, static_cast<u16>(y + 1u), 0, 0, 0);
    }
}

static void paint_imp (u8* rgb, u16 w, u16 h, u16 x, u16 y, const GameState& state) {
    const u8 typ = state.m_map.get_add_typ(x, y);
    if (typ == BUILD_ADD_MINE) {
        set_px(rgb, w, h, x, y, 220, 180, 40);
        return;
    }
    if (typ == BUILD_ADD_PLANTATION) {
        set_px(rgb, w, h, x, y, 160, 60, 200);
        return;
    }
    if (typ != BUILD_ADD_STD) {
        return;
    }
    const GameTileSimple* t = state.m_map.tile(x, y);
    if (StdAddHelper::has_farm(t)) {
        set_px(rgb, w, h, x, y, 220, 30, 30);
    } else if (StdAddHelper::has_mill(t)) {
        set_px(rgb, w, h, x, y, 40, 160, 40);
    } else if (StdAddHelper::has_irr(t)) {
        set_px(rgb, w, h, x, y, 40, 140, 220);
    }
}

static u32 count_imp_on_map (const GameState& state, u16 imp_idx) {
    if (state.m_statics == nullptr || imp_idx >= state.m_statics->worker_job_imp().get_item_count()) {
        return 0u;
    }
    const RuntimeStatics& st = *state.m_statics;
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u32 n = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const GameTileSimple* t = state.m_map.tile(x, y);
            if (TileImpHelper::has_imp(t, st, imp_idx)) {
                n = n + 1u;
            }
        }
    }
    return n;
}

static u32 count_overlay_on_map (const GameState& state, u16 ov_idx) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u32 n = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (state.m_map.get_overlay(x, y) == ov_idx) {
                n = n + 1u;
            }
        }
    }
    return n;
}

static void print_overlay_imps_on_map (const GameState& state, MapOverlay ov) {
    if (state.m_statics == nullptr) {
        return;
    }
    const RuntimeStatics& st = *state.m_statics;
    const u16 ov_i = static_cast<u16>(ov);
    cstr ov_nm = st.map_overlay().get_name(MapOverlayStaticDataKey::from_raw(ov_i));
    if (ov_nm == nullptr) {
        ov_nm = "unknown_overlay";
    }
    const u32 ov_tiles = count_overlay_on_map(state, ov_i);
    std::printf("  %s: %u\n", ov_nm, (unsigned)ov_tiles);
    const WorkerJobImpIndex& ix = st.worker_job_imp_index();
    const u16 imp_n = ix.imp_n(ov_i);
    const u16* imp_ids = ix.imps(ov_i);
    if (imp_ids == nullptr || imp_n == 0u) {
        return;
    }
    for (u16 i = 0; i < imp_n; ++i) {
        const u32 cnt = count_imp_on_map(state, imp_ids[i]);
        if (cnt == 0u) {
            continue;
        }
        cstr inm = st.worker_job_imp().get_name(WorkerJobImpStaticDataKey::from_raw(imp_ids[i]));
        if (inm == nullptr) {
            inm = "unknown_imp";
        }
        std::printf("    %s: %u\n", inm, (unsigned)cnt);
    }
}

static void print_imps_on_map (const GameState& state) {
    std::printf(" imps on map:\n");
    print_overlay_imps_on_map(state, MapOverlay::Farm);
    print_overlay_imps_on_map(state, MapOverlay::Forest);
    print_overlay_imps_on_map(state, MapOverlay::Mine);
    print_overlay_imps_on_map(state, MapOverlay::Plantation);
    print_overlay_imps_on_map(state, MapOverlay::Fort);
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

static u32 count_roads (const GameState& state) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    u32 n = 0u;
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (road_is_built(state.m_map.get_road_typ(x, y))) {
                n = n + 1u;
            }
        }
    }
    return n;
}

static bool save_terrain_roads_ppm (const GameState& state, u32 turn) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            terr_rgb(state.m_map.get_terrain(x, y), &r, &g, &b);
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 rd = state.m_map.get_road_typ(x, y);
            if (road_is_virtual(rd)) {
                set_px(rgb, w, h, x, y, 96, 96, 96);
            } else if (road_is_built(rd)) {
                set_px(rgb, w, h, x, y, 48, 48, 48);
            }
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        set_px(rgb, w, h, c->get_x(), c->get_y(), 0, 0, 0);
    }
    char path[512];
    std::snprintf(path, sizeof(path), "%s/terrain_roads_t%03u.ppm", G_OUT_DIR, (unsigned)turn);
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    if (ok) {
        std::printf("wrote %s roads=%u\n", path, (unsigned)count_roads(state));
    }
    return ok;
}

static bool save_turn_ppm (const GameState& state, u32 turn) {
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    if (w == 0 || h == 0) {
        return false;
    }
    const u32 n = static_cast<u32>(w) * static_cast<u32>(h);
    u8* rgb = new u8[static_cast<size_t>(n) * 3u];
    if (rgb == nullptr) {
        return false;
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            u8 r = 0;
            u8 g = 0;
            u8 b = 0;
            climate_to_rgb(state.m_map.get_climate(x, y), &r, &g, &b);
            if (state.m_map.get_river(x, y) != 0) {
                r = 40;
                g = 100;
                b = 220;
            }
            if (state.m_map.get_terrain(x, y) == TERR_MOUNTAINS[0]) {
                r = 120;
                g = 72;
                b = 40;
            }
            set_px(rgb, w, h, x, y, r, g, b);
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = state.m_map.get_civ_owner(x, y);
            if (own != U8_KEY_NULL) {
                shade_own(rgb, w, h, x, y, static_cast<u16>(own));
            }
        }
    }
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            paint_imp(rgb, w, h, x, y, state);
        }
    }
    const u16 cn = state.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = state.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() == U16_KEY_NULL) {
            continue;
        }
        paint_black_mark(rgb, w, h, c->get_x(), c->get_y());
    }
    const u32 scan_n = static_cast<u32>(state.m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddStruct* u = state.m_units.get_unit_add(UnitAddKey::from_raw(static_cast<u16>(idx)));
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            continue;
        }
        if (!is_worker_typ(*state.m_statics, u->m_unit_typ_idx)) {
            continue;
        }
        set_px(rgb, w, h, u->m_x, u->m_y, 128, 0, 0);
    }
    char path[384];
    if (std::snprintf(path, sizeof(path), "%s/turn_%04u.ppm", G_OUT_DIR, turn) <= 0) {
        delete[] rgb;
        return false;
    }
    std::FILE* fp = std::fopen(path, "wb");
    if (fp == nullptr) {
        delete[] rgb;
        return false;
    }
    std::fprintf(fp, "P6\n%u %u\n255\n", static_cast<unsigned>(w), static_cast<unsigned>(h));
    const size_t nbytes = static_cast<size_t>(n) * 3u;
    const bool ok = std::fwrite(rgb, 1, nbytes, fp) == nbytes;
    std::fclose(fp);
    delete[] rgb;
    return ok;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main (int argc, char** argv) {
    bool extend = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "extend") == 0) {
            extend = true;
        }
        if (std::strcmp(argv[i], "time_only") == 0) {
            g_time_only = true;
        }
        if (std::strcmp(argv[i], "tech") == 0) {
            g_gradual_tech = true;
        }
    }
    const u32 turn_cap = extend ? G_TURN_CAP_EXT : G_TURN_CAP;
#ifndef CITY_TRACER_ENABLE
    std::printf("CITY_TRACER_ENABLE not set; rebuild with -DCITY_TRACER_ENABLE\n");
    return 1;
#endif
    if (!build_paths()) {
        std::printf("path build failed\n");
        return 1;
    }
    if (!ensure_out_dir()) {
        std::printf("out dir failed\n");
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
    if (state.m_statics == nullptr) {
        std::printf("missing statics\n");
        state.clear();
        return 1;
    }
    const RuntimeStatics& st = *state.m_statics;
    BitArrayCL tech(st.tech().get_item_count());
    if (g_gradual_tech) {
        ensure_tech_arrays(state);
        const u32 tech_n = tech.get_count();
        g_tech_cur = 0;
        g_tech_iv = (tech_n == 0u) ? turn_cap : (turn_cap / tech_n);
        if (g_tech_iv == 0u) {
            g_tech_iv = 1u;
        }
    } else {
        unlock_all_tech(state);
        for (u32 i = 0; i < tech.get_count(); ++i) {
            tech.set_bit(i);
        }
        g_tech_cur = tech.get_count();
    }
    TileYieldCtx yctx = {};
    yctx.m_tech = &tech;
    TileYields::bind_ctx(&yctx);
    TileWorkCtx wctx = {};
    wctx.m_tech = &tech;
    wctx.m_resource = nullptr;
    TileWorkAssessor::bind_ctx(&wctx);

    const u16 worker_typ = find_worker_typ(st);
    if (worker_typ == U16_KEY_NULL) {
        std::printf("fail find worker typ\n");
        state.clear();
        setup.release_map_gen();
        return 1;
    }

    WhiteboardMng::init(state.m_map.width(), state.m_map.height());
    claim_city_borders(state);
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        ps.m_target_settlements = SETTLER_MISSION_SLOTS;
        ps.m_worker_tile_opt_scan = 1;
        ps.m_worker_tile_opt_reassign = 0;
    }

    g_st = &st;
    g_state = &state;
    g_job_apps = 0;
    g_tile_skip_viol = 0;
    g_unit_tile.clear();
    g_job_tot.clear();
    WorkerTurnHandler::set_job_note(on_job);

    state.m_turn_limit = turn_cap;
    state.m_current_turn = 0;
    const u16 cities0 = count_cities(state);
    g_city_spawned_n = 0;
    spawn_workers_for_new_cities(state, worker_typ);
    const u16 workers0 = count_workers(state);
    std::printf("*** start players=%u cities=%u workers=%u turn_cap=%u scan=%u reassign=%u workers_per_city=%u extend=%d ppm_every=%u time_only=%d tech=%d tech_iv=%u\n",
        state.m_player_n, cities0, workers0, turn_cap,
        (unsigned)state.m_player_states[0].m_worker_tile_opt_scan,
        (unsigned)state.m_player_states[0].m_worker_tile_opt_reassign,
        (unsigned)G_WORKERS_PER_CITY,
        extend ? 1 : 0, g_ppm_every, g_time_only ? 1 : 0,
        g_gradual_tech ? 1 : 0, g_tech_iv);
    if (!g_time_only) {
        if (!save_turn_ppm(state, 0)) {
            std::printf("save turn 0 failed\n");
            WorkerTurnHandler::set_job_note(nullptr);
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
        if (!save_terrain_roads_ppm(state, 0)) {
            std::printf("save terrain roads turn 0 failed\n");
            WorkerTurnHandler::set_job_note(nullptr);
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
    }

    GameLoop loop;
    LOG_CITY_SETUP((G_CITY_LOG));
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("GameLoop::begin failed\n");
        LOG_CITY_CLEAR(());
        WorkerTurnHandler::set_job_note(nullptr);
        state.clear();
        return 1;
    }
    for (u16 p = 0; p < state.m_player_n; ++p) {
        PlayerState& ps = state.m_player_states[p];
        ps.m_target_settlements = SETTLER_MISSION_SLOTS;
        ps.m_worker_tile_opt_scan = 1;
        ps.m_worker_tile_opt_reassign = 0;
    }
    spawn_workers_for_new_cities(state, worker_typ);

    const auto t_loop0 = std::chrono::steady_clock::now();
    while (state.m_current_turn < turn_cap) {
        state.m_current_turn = state.m_current_turn + 1u;
        gradual_tech_turn(state, tech, state.m_current_turn);
        {
            const auto t0 = std::chrono::steady_clock::now();
            run_city_turns(state);
            run_unit_turns(state);
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_turn, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }
        {
            const auto t0 = std::chrono::steady_clock::now();
            spawn_workers_for_new_cities(state, worker_typ);
            const auto t1 = std::chrono::steady_clock::now();
            tm_add(&g_tm_spawn, static_cast<u64>(
                std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
        }
        if (g_time_only) {
            std::printf("\rturn %u / %u", state.m_current_turn, turn_cap);
            std::fflush(stdout);
        } else if (g_ppm_every != 0 && (state.m_current_turn % g_ppm_every) == 0) {
            if (!save_turn_ppm(state, state.m_current_turn)) {
                std::printf("save turn %u failed\n", state.m_current_turn);
                loop.end();
                LOG_CITY_CLEAR(());
                WorkerTurnHandler::set_job_note(nullptr);
                SettlerTurnHandler::clear();
                state.clear();
                return 1;
            }
            if (!save_terrain_roads_ppm(state, state.m_current_turn)) {
                std::printf("save terrain roads turn %u failed\n", state.m_current_turn);
                loop.end();
                LOG_CITY_CLEAR(());
                WorkerTurnHandler::set_job_note(nullptr);
                SettlerTurnHandler::clear();
                state.clear();
                return 1;
            }
            u32 irrs = count_imp_on_map(state, static_cast<u16>(WorkerJobImp::Irrigation));
            std::printf("t=%u cities=%u workers=%u farm_ov=%u irr=%u jobs=%u\n",
                (unsigned)state.m_current_turn,
                (unsigned)count_cities(state),
                (unsigned)count_workers(state),
                (unsigned)count_overlay_on_map(state, static_cast<u16>(MapOverlay::Farm)),
                (unsigned)irrs,
                (unsigned)g_job_apps);
        }
    }
    const auto t_loop1 = std::chrono::steady_clock::now();
    if (g_time_only) {
        std::printf("\n");
    }
    if (!save_turn_ppm(state, state.m_current_turn)) {
        std::printf("save final turn failed\n");
        loop.end();
        LOG_CITY_CLEAR(());
        WorkerTurnHandler::set_job_note(nullptr);
        SettlerTurnHandler::clear();
        state.clear();
        return 1;
    }
    if (!save_terrain_roads_ppm(state, state.m_current_turn)) {
        std::printf("save final terrain roads failed\n");
        loop.end();
        LOG_CITY_CLEAR(());
        WorkerTurnHandler::set_job_note(nullptr);
        SettlerTurnHandler::clear();
        state.clear();
        return 1;
    }
    loop.end();
    LOG_CITY_CLEAR(());
    WorkerTurnHandler::set_job_note(nullptr);

    const u16 cities1 = count_cities(state);
    const u16 workers1 = count_workers(state);
    const u32 irrs = count_imp_on_map(state, static_cast<u16>(WorkerJobImp::Irrigation));
    const bool a_turns = state.m_current_turn == turn_cap;
    const bool a_jobs = g_job_apps > 0;
    const bool a_irr = irrs > 0;
    const bool a_skip = g_tile_skip_viol == 0u;
    const bool a_tech = !g_gradual_tech || g_tech_cur >= tech.get_count();
    const bool ok = a_turns && a_jobs && a_irr && a_skip && a_tech;
    const double loop_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_loop1 - t_loop0).count()) / 1.0e6;
    const double avg_ms = (turn_cap == 0) ? 0.0 : loop_ms / static_cast<double>(turn_cap);

    std::printf("=======================================================\n");
    note_assert(a_turns, "reached turn_cap");
    note_assert(a_jobs, "jobs_applied > 0");
    note_assert(a_irr, "Irrigation present on map");
    {
        char skip_msg[64];
        std::snprintf(skip_msg, sizeof(skip_msg), "tile_skip_violations == 0 (%u)",
            (unsigned)g_tile_skip_viol);
        note_assert(a_skip, skip_msg);
    }
    note_assert(a_tech, "tech unlock complete (or gradual off)");
    note_assert(ok, "WORKER TURN MNG overall");
    std::printf(" WORKER TURN MNG: %s after %u turns (players=%u cities %u -> %u workers %u -> %u)\n",
        ok ? "PASS" : "FAIL", state.m_current_turn, state.m_player_n, cities0, cities1, workers0, workers1);
    std::printf(" imps: jobs_applied=%u  tile_skip_violations=%u  tech_unlocked=%u/%u\n",
        (unsigned)g_job_apps, (unsigned)g_tile_skip_viol,
        (unsigned)g_tech_cur, (unsigned)tech.get_count());
    print_imps_on_map(state);
    std::printf(" loop wall: %.3f ms total  %.3f ms/turn (includes tester spawn + ppm)\n", loop_ms, avg_ms);
    std::printf(" turn e2e:  %.3f ms total  %.3f ms/turn (city+unit only)\n",
        static_cast<double>(g_tm_turn.ns) / 1.0e6,
        (g_tm_turn.n == 0) ? 0.0 : (static_cast<double>(g_tm_turn.ns) / static_cast<double>(g_tm_turn.n)) / 1.0e6);
    std::printf(" maps: %s/turn_XXXX.ppm\n", G_OUT_DIR);
    std::printf(" cities.trace: %s\n", G_CITY_LOG);
    std::printf(" hot-path timings:\n");
    tm_report_all();
    print_job_tots();
    std::printf("=======================================================\n");

    g_st = nullptr;
    g_state = nullptr;
    TileWorkAssessor::bind_ctx(nullptr);
    TileYields::bind_ctx(nullptr);
    SettlerTurnHandler::clear();
    WhiteboardMng::terminate();
    state.clear();
    setup.release_map_gen();
    return ok ? 0 : 1;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
