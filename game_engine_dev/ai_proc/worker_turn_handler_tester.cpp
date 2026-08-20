//================================================================================================================================
//=> - Includes -
//================================================================================================================================

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
#include "city_turn_handler.h"
#include "defensive_unit_turn_handler.h"
#include "game_loop.h"
#include "game_map_defs.h"
#include "game_setup.h"
#include "game_state.h"
#include "research_turn_handler.h"
#include "runtime_statics.h"
#include "settler_turn_handler.h"
#include "std_add_helper.h"
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
#include "worker_turn_handler.h"

//================================================================================================================================
//=> - Constants -
//================================================================================================================================

typedef const char* cstr;

static const char* G_MAP_ROOT = "/home/w/Projects/simple-map-gen";
static const char* G_OUT_DIR = "/home/w/Projects/simple-map-gen/worker-turn-mng";
static const char* G_TRACE = "/home/w/Projects/simple-map-gen/worker-turn-mng/game_loop.trace";
static const u32 G_SEED = 43u;
static const u16 G_PLAYERS = 100;
static const u32 G_TURN_CAP = 300u;
static const u32 G_TURN_CAP_EXT = 1000u;
static const u16 G_CLAIM_CULT = 25u;
static u32 g_ppm_every = 10u;
static bool g_time_only = false;
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
    std::string m_nm;
    u32 m_n;
};

static std::vector<JobTot> g_job_tot;
static const RuntimeStatics* g_st = nullptr;
static u32 g_job_apps = 0;

static void on_job (u16 x, u16 y, u16 job, u8 intent) {
    (void)x;
    (void)y;
    (void)intent;
    if (g_st == nullptr) {
        return;
    }
    cstr nm = g_st->worker_job().get_name(WorkerJobStaticDataKey::from_raw(job));
    if (nm == nullptr) {
        nm = "unknown_job";
    }
    g_job_apps = g_job_apps + 1u;
    for (size_t i = 0; i < g_job_tot.size(); ++i) {
        if (g_job_tot[i].m_nm == nm) {
            g_job_tot[i].m_n = g_job_tot[i].m_n + 1u;
            return;
        }
    }
    JobTot t;
    t.m_nm = nm;
    t.m_n = 1u;
    g_job_tot.push_back(t);
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
    const u16 pts = state.m_statics->unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).mvt_pts;
    u->m_mvt_points = static_cast<i16>(pts * state.m_statics->config().get_mov_pt_per_turn());
}

static void after_city_turns (GameState& state) {
    for (u16 p = 0; p < state.m_player_n; ++p) {
        const auto t0 = std::chrono::steady_clock::now();
        ResearchTurnHandler::handle(state, p);
        const auto t1 = std::chrono::steady_clock::now();
        tm_add(&g_tm_research, static_cast<u64>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count()));
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
            const auto t0 = std::chrono::steady_clock::now();
            WorkerTurnHandler::handle(state, unit_idx);
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
    if (typ != BUILD_ADD_STD || state.m_map.get_add_idx(x, y) == U16_KEY_NULL) {
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

static void count_imps (const GameState& state, u32* farms, u32* mills, u32* irrs, u32* mines, u32* plants) {
    *farms = 0;
    *mills = 0;
    *irrs = 0;
    *mines = 0;
    *plants = 0;
    const u16 w = state.m_map.width();
    const u16 h = state.m_map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 typ = state.m_map.get_add_typ(x, y);
            if (typ == BUILD_ADD_MINE) {
                *mines = *mines + 1u;
                continue;
            }
            if (typ == BUILD_ADD_PLANTATION) {
                *plants = *plants + 1u;
                continue;
            }
            if (typ != BUILD_ADD_STD || state.m_map.get_add_idx(x, y) == U16_KEY_NULL) {
                continue;
            }
            const GameTileSimple* t = state.m_map.tile(x, y);
            if (StdAddHelper::has_farm(t)) {
                *farms = *farms + 1u;
            }
            if (StdAddHelper::has_mill(t)) {
                *mills = *mills + 1u;
            }
            if (StdAddHelper::has_irr(t)) {
                *irrs = *irrs + 1u;
            }
        }
    }
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
    }
    const u32 turn_cap = extend ? G_TURN_CAP_EXT : G_TURN_CAP;
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
    unlock_all_tech(state);
    BitArrayCL tech(st.tech().get_item_count());
    for (u32 i = 0; i < tech.get_count(); ++i) {
        tech.set_bit(i);
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
    g_job_apps = 0;
    g_job_tot.clear();
    WorkerTurnHandler::set_job_note(on_job);

    state.m_turn_limit = turn_cap;
    state.m_current_turn = 0;
    const u16 cities0 = count_cities(state);
    g_city_spawned_n = 0;
    spawn_workers_for_new_cities(state, worker_typ);
    const u16 workers0 = count_workers(state);
    std::printf("*** start players=%u cities=%u workers=%u turn_cap=%u scan=%u reassign=%u workers_per_city=%u extend=%d ppm_every=%u time_only=%d\n",
        state.m_player_n, cities0, workers0, turn_cap,
        (unsigned)state.m_player_states[0].m_worker_tile_opt_scan,
        (unsigned)state.m_player_states[0].m_worker_tile_opt_reassign,
        (unsigned)G_WORKERS_PER_CITY,
        extend ? 1 : 0, g_ppm_every, g_time_only ? 1 : 0);
    if (!g_time_only) {
        if (!save_turn_ppm(state, 0)) {
            std::printf("save turn 0 failed\n");
            WorkerTurnHandler::set_job_note(nullptr);
            WhiteboardMng::terminate();
            state.clear();
            return 1;
        }
    }

    GameLoop loop;
    if (!loop.begin(&state, G_TRACE)) {
        std::printf("GameLoop::begin failed\n");
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
                WorkerTurnHandler::set_job_note(nullptr);
                SettlerTurnHandler::clear();
                state.clear();
                return 1;
            }
            u32 farms = 0;
            u32 mills = 0;
            u32 irrs = 0;
            u32 mines = 0;
            u32 plants = 0;
            count_imps(state, &farms, &mills, &irrs, &mines, &plants);
            std::printf("t=%u cities=%u workers=%u farms=%u mills=%u irr=%u mines=%u plants=%u jobs=%u\n",
                (unsigned)state.m_current_turn,
                (unsigned)count_cities(state),
                (unsigned)count_workers(state),
                (unsigned)farms, (unsigned)mills, (unsigned)irrs, (unsigned)mines, (unsigned)plants,
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
        WorkerTurnHandler::set_job_note(nullptr);
        SettlerTurnHandler::clear();
        state.clear();
        return 1;
    }
    loop.end();
    WorkerTurnHandler::set_job_note(nullptr);

    const u16 cities1 = count_cities(state);
    const u16 workers1 = count_workers(state);
    u32 farms = 0;
    u32 mills = 0;
    u32 irrs = 0;
    u32 mines = 0;
    u32 plants = 0;
    count_imps(state, &farms, &mills, &irrs, &mines, &plants);
    const bool ok = state.m_current_turn == turn_cap && g_job_apps > 0;
    const double loop_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t_loop1 - t_loop0).count()) / 1.0e6;
    const double avg_ms = (turn_cap == 0) ? 0.0 : loop_ms / static_cast<double>(turn_cap);

    std::printf("=======================================================\n");
    std::printf(" WORKER TURN MNG: %s after %u turns (players=%u cities %u -> %u workers %u -> %u)\n",
        ok ? "PASS" : "FAIL", state.m_current_turn, state.m_player_n, cities0, cities1, workers0, workers1);
    std::printf(" imps: farms=%u mills=%u irr=%u mines=%u plants=%u  jobs_applied=%u\n",
        (unsigned)farms, (unsigned)mills, (unsigned)irrs, (unsigned)mines, (unsigned)plants, (unsigned)g_job_apps);
    std::printf(" loop wall: %.3f ms total  %.3f ms/turn (includes tester spawn + ppm)\n", loop_ms, avg_ms);
    std::printf(" turn e2e:  %.3f ms total  %.3f ms/turn (city+unit only)\n",
        static_cast<double>(g_tm_turn.ns) / 1.0e6,
        (g_tm_turn.n == 0) ? 0.0 : (static_cast<double>(g_tm_turn.ns) / static_cast<double>(g_tm_turn.n)) / 1.0e6);
    std::printf(" maps: %s/turn_XXXX.ppm\n", G_OUT_DIR);
    std::printf(" hot-path timings:\n");
    tm_report_all();
    std::printf(" totals by job:\n");
    for (size_t i = 0; i < g_job_tot.size(); ++i) {
        std::printf("  %s: %u\n", g_job_tot[i].m_nm.c_str(), (unsigned)g_job_tot[i].m_n);
    }
    std::printf("=======================================================\n");

    g_st = nullptr;
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
