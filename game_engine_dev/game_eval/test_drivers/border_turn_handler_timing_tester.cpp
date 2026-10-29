//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "border_turn_handler.h"
#include "city_array.h"
#include "eval_driver.h"
#include "eval_need.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 k_one_lim = 10u;
static const u16 k_all_n = 10u;

//================================================================================================================================
//=> - Timing -
//================================================================================================================================

struct TimeAcc {
    u32 m_n;
    double m_min;
    double m_max;
    double m_sum;

    void clr () {
        m_n = 0u;
        m_min = 0.0;
        m_max = 0.0;
        m_sum = 0.0;
    }

    void add (double ms) {
        if (m_n == 0u) {
            m_min = ms;
            m_max = ms;
            m_sum = ms;
            m_n = 1u;
            return;
        }
        if (ms < m_min) {
            m_min = ms;
        }
        if (ms > m_max) {
            m_max = ms;
        }
        m_sum = m_sum + ms;
        m_n = m_n + 1u;
    }

    double avg () const {
        return (m_n == 0u) ? 0.0 : (m_sum / static_cast<double>(m_n));
    }
};

static void free_seats (PlayerState*& seats, u16& n) {
    if (seats == nullptr) {
        n = 0;
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        delete[] seats[i].m_small_wonder_city;
        seats[i].m_small_wonder_city = nullptr;
        delete seats[i].m_explored_overlay;
        seats[i].m_explored_overlay = nullptr;
        delete seats[i].m_techs_researched;
        seats[i].m_techs_researched = nullptr;
        delete seats[i].m_civics_adopted;
        seats[i].m_civics_adopted = nullptr;
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

static void set_all_need (BorderFront* fronts, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        fronts[i].m_city = BorderTurnHandler::k_front_need;
        fronts[i].m_pl = U16_KEY_NULL;
    }
}

static void set_one_need (BorderFront* fronts, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        fronts[i].m_city = BorderTurnHandler::k_front_idle;
        fronts[i].m_pl = U16_KEY_NULL;
    }
    fronts[0].m_city = BorderTurnHandler::k_front_need;
    fronts[0].m_pl = U16_KEY_NULL;
}

static void wr_acc (cstr mk, cstr mode, const TimeAcc& a) {
    std::printf("border_timing %s %s n=%u min_ms=%.4f max_ms=%.4f avg_ms=%.4f\n",
        mk, mode, a.m_n, a.m_min, a.m_max, a.avg());
}

//================================================================================================================================
//=> - BorderTurnHandlerTimingTester -
//================================================================================================================================

class BorderTurnHandlerTimingTester : public EvalDriver {
public:
    BorderTurnHandlerTimingTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

BorderTurnHandlerTimingTester::BorderTurnHandlerTimingTester ()
    : EvalDriver(make_need()) {
}

int BorderTurnHandlerTimingTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n == 0u) {
        std::printf("border_timing: no saves\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("border_timing: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("border_timing: TechAgeMng::setup failed\n");
        return 1;
    }
    const char* mk = std::getenv("BORDER_MK");
    if (mk == nullptr || mk[0] == 0) {
        mk = "mk06";
    }
    TimeAcc all_acc;
    TimeAcc one_acc;
    all_acc.clr();
    one_acc.clr();
    u32 seat_ok = 0u;
    for (u16 si = 0; si < save_n; ++si) {
        const u32 turn = paths().save_turn_at(si);
        char map_p[512];
        char cities_p[512];
        char players_p[512];
        if (!paths().map_path(turn, map_p, sizeof(map_p))
            || !paths().cities_path(turn, cities_p, sizeof(cities_p))
            || !paths().players_path(turn, players_p, sizeof(players_p))) {
            std::printf("border_timing: bad paths turn=%u\n", turn);
            TechAgeMng::clear();
            return 1;
        }
        GameArraySimple map;
        if (!GameIo::load_map_tiles(map_p, map)) {
            std::printf("border_timing: load_map failed %s\n", map_p);
            TechAgeMng::clear();
            return 1;
        }
        CityArray cities;
        if (!GameIo::load_cities(cities_p, cities)) {
            std::printf("border_timing: load_cities failed %s\n", cities_p);
            TechAgeMng::clear();
            return 1;
        }
        PlayerState* seats = nullptr;
        u16 seat_n = 0;
        if (!GameIo::load_players(players_p, seats, seat_n)) {
            std::printf("border_timing: load_players failed %s\n", players_p);
            TechAgeMng::clear();
            return 1;
        }
        BorderTurnHandler bth;
        if (!bth.begin(map, cities)) {
            std::printf("border_timing: begin failed turn=%u\n", turn);
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        for (u16 seat = 0; seat < seat_n; ++seat) {
            if (seats[seat].m_lucky == 0u) {
                continue;
            }
            BorderFront fronts[BorderTurnHandler::k_front_cap];
            set_all_need(fronts, k_all_n);
            const auto t0 = std::chrono::steady_clock::now();
            const bool ok_all = bth.handle(seat, fronts, k_all_n);
            const auto t1 = std::chrono::steady_clock::now();
            if (!ok_all) {
                continue;
            }
            all_acc.add(std::chrono::duration<double, std::milli>(t1 - t0).count());

            double one_sum = 0.0;
            u16 one_n = 0u;
            bool one_ok = true;
            for (u16 p = 0; p < k_one_lim; ++p) {
                set_one_need(fronts, k_all_n);
                const auto u0 = std::chrono::steady_clock::now();
                if (!bth.handle(seat, fronts, k_all_n)) {
                    one_ok = false;
                    break;
                }
                const auto u1 = std::chrono::steady_clock::now();
                one_sum = one_sum + std::chrono::duration<double, std::milli>(u1 - u0).count();
                one_n = static_cast<u16>(one_n + 1u);
            }
            if (!one_ok || one_n == 0u) {
                continue;
            }
            one_acc.add(one_sum / static_cast<double>(one_n));
            seat_ok = seat_ok + 1u;
        }
        free_seats(seats, seat_n);
    }
    TechAgeMng::clear();
    if (WhiteboardMng::chkout() == 0u
        && (WhiteboardMng::width() != 0u || WhiteboardMng::height() != 0u)) {
        WhiteboardMng::terminate();
    }
    wr_acc(mk, "all-at-once", all_acc);
    wr_acc(mk, "one-at-a-time", one_acc);
    std::printf("border_timing %s ok saves=%u seats=%u one_lim=%u\n",
        mk, static_cast<u32>(save_n), seat_ok, static_cast<u32>(k_one_lim));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    BorderTurnHandlerTimingTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
