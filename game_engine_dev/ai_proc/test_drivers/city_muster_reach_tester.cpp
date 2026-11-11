//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstring>

#include "city.h"
#include "city_array.h"
#include "city_muster_grad.h"
#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"
#include "unit_movement_mng.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_LIB = "../../data_io/runtime_static_loader_lib.so";
static const char* G_DATA = "../../";

//================================================================================================================================
//=> - Timing helpers -
//================================================================================================================================

static const u16 k_seat_cap = 256u;
static const u16 k_buck_n = 7u;
static const u64 k_buck_hi[k_buck_n] = {
    100ull, 1000ull, 10000ull, 100000ull, 1000000ull, 10000000ull, 0ull
};

static cstr time_unit (u8 u) {
    if (u == 0u) {
        return "s";
    }
    if (u == 1u) {
        return "ms";
    }
    if (u == 2u) {
        return "us";
    }
    return "ns";
}

static double time_div (u8 u) {
    if (u == 0u) {
        return 1000000000.0;
    }
    if (u == 1u) {
        return 1000000.0;
    }
    if (u == 2u) {
        return 1000.0;
    }
    return 1.0;
}

static u8 pick_time_unit (double ns) {
    for (u8 u = 0u; u < 4u; ++u) {
        if ((ns / time_div(u)) > 1.0) {
            return u;
        }
    }
    return 3u;
}

static void print_ns (cstr label, double ns) {
    const u8 u = pick_time_unit(ns);
    std::printf("%s%.2f %s", label, ns / time_div(u), time_unit(u));
}

static u16 buck_i (u64 ns) {
    for (u16 i = 0; i < k_buck_n - 1u; ++i) {
        if (ns < k_buck_hi[i]) {
            return i;
        }
    }
    return static_cast<u16>(k_buck_n - 1u);
}

static void print_buck_edge (u64 ns) {
    const u8 u = pick_time_unit(static_cast<double>(ns == 0ull ? 1ull : ns));
    std::printf("%.2f %s", static_cast<double>(ns) / time_div(u), time_unit(u));
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    EvalPaths paths;
    if (!paths.load(G_PATHS) || !paths.scan_saves() || paths.save_turn_n() == 0u) {
        std::printf("city_muster_reach: no saves\n");
        return 1;
    }
    const u32 turn = paths.save_turn_at(static_cast<u16>(paths.save_turn_n() - 1u));
    char map_p[512];
    char cities_p[512];
    char players_p[512];
    if (!paths.map_path(turn, map_p, sizeof(map_p))
        || !paths.cities_path(turn, cities_p, sizeof(cities_p))
        || !paths.players_path(turn, players_p, sizeof(players_p))) {
        std::printf("city_muster_reach: path fail turn=%u\n", turn);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_LIB, G_DATA)) {
        std::printf("city_muster_reach: statics load fail\n");
        return 1;
    }
    RuntimeStatics& rst = loader.statics();
    if (!TechAgeMng::setup(rst) || !UnitMovementMng::setup_mvt_costs(rst)) {
        std::printf("city_muster_reach: setup fail\n");
        TechAgeMng::clear();
        return 1;
    }
    GameState st;
    st.m_statics = &rst;
    if (!st.m_cities.bind_statics(rst)
        || !GameIo::load_map_tiles(map_p, st.m_map)
        || !GameIo::load_cities(cities_p, st.m_cities)
        || !GameIo::load_players(players_p, st)) {
        std::printf("city_muster_reach: load fail turn=%u\n", turn);
        TechAgeMng::clear();
        return 1;
    }
    st.m_statics = &rst;
    st.m_current_turn = turn;
    UnitMovementMng::bind_state(&st);
    WhiteboardMng::init(st.m_map.width(), st.m_map.height());
    CityMusterGrad cm;
    if (!cm.bind(&st) || !cm.set_caps() || !cm.build() || !cm.ok()) {
        std::printf("city_muster_reach: muster build fail turn=%u\n", turn);
        WhiteboardMng::terminate();
        TechAgeMng::clear();
        return 1;
    }
    u16 seat_n[k_seat_cap];
    u16 seat_ok[k_seat_cap];
    for (u16 s = 0; s < k_seat_cap; ++s) {
        seat_n[s] = 0u;
        seat_ok[s] = 0u;
    }
    u32 buck[k_buck_n];
    for (u16 i = 0; i < k_buck_n; ++i) {
        buck[i] = 0u;
    }
    u64 sum_ns = 0u;
    u64 min_ns = ~0ull;
    u64 max_ns = 0u;
    u32 walk_n = 0u;
    u32 conn_n = 0u;
    const u16 cn = st.m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = st.m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 own = c->get_owner();
        if (own >= st.m_player_n || own >= k_seat_cap) {
            continue;
        }
        if (st.m_player_states[own].m_muster_city == U16_KEY_NULL) {
            continue;
        }
        seat_n[own] = static_cast<u16>(seat_n[own] + 1u);
        const auto t0 = std::chrono::steady_clock::now();
        const bool ok = cm.city_reach(i);
        const auto t1 = std::chrono::steady_clock::now();
        const u64 ns = static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
        buck[buck_i(ns)] += 1u;
        sum_ns += ns;
        if (ns < min_ns) {
            min_ns = ns;
        }
        if (ns > max_ns) {
            max_ns = ns;
        }
        walk_n += 1u;
        if (ok) {
            conn_n += 1u;
            seat_ok[own] = static_cast<u16>(seat_ok[own] + 1u);
        }
    }
    WhiteboardMng::terminate();
    TechAgeMng::clear();
    std::printf("city_muster_reach turn=%u cities=%u walks=%u connected=%u (%.2f%%)\n",
        turn,
        static_cast<unsigned>(cn),
        static_cast<unsigned>(walk_n),
        static_cast<unsigned>(conn_n),
        walk_n == 0u ? 0.0 : (100.0 * static_cast<double>(conn_n) / static_cast<double>(walk_n)));
    if (walk_n > 0u) {
        print_ns("  walk min=", static_cast<double>(min_ns));
        std::printf("\n");
        print_ns("  walk max=", static_cast<double>(max_ns));
        std::printf("\n");
        print_ns("  walk avg=", static_cast<double>(sum_ns) / static_cast<double>(walk_n));
        std::printf("\n");
    }
    std::printf("city_muster_reach walk time buckets\n");
    u64 lo = 0ull;
    for (u16 i = 0; i < k_buck_n; ++i) {
        std::printf("  ");
        if (i + 1u == k_buck_n) {
            print_buck_edge(lo);
            std::printf("+ : %u\n", static_cast<unsigned>(buck[i]));
        } else {
            print_buck_edge(lo);
            std::printf(" .. ");
            print_buck_edge(k_buck_hi[i]);
            std::printf(" : %u\n", static_cast<unsigned>(buck[i]));
            lo = k_buck_hi[i];
        }
    }
    std::printf("city_muster_reach seat connected%%\n");
    for (u16 s = 0; s < st.m_player_n && s < k_seat_cap; ++s) {
        if (seat_n[s] == 0u) {
            continue;
        }
        const double pct = 100.0 * static_cast<double>(seat_ok[s]) / static_cast<double>(seat_n[s]);
        std::printf("  seat=%u cities=%u connected=%u (%.2f%%) muster=%u\n",
            static_cast<unsigned>(s),
            static_cast<unsigned>(seat_n[s]),
            static_cast<unsigned>(seat_ok[s]),
            pct,
            static_cast<unsigned>(st.m_player_states[s].m_muster_city));
    }
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
