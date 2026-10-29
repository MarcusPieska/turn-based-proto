//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cstdint>
#include <sys/stat.h>
#include <sys/types.h>

#include "border_turn_handler.h"
#include "city.h"
#include "city_array.h"
#include "eval_driver.h"
#include "eval_need.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "map_own_save_snap.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_PLOT = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/annotate_map.py";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 k_seat_cap = 256u;
static const i32 k_box = 50;
static const i32 k_half = 25;

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static bool mkdir_p (cstr path) {
    if (path == nullptr) {
        return false;
    }
    return ::mkdir(path, 0755) == 0 || errno == EEXIST;
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
    for (u32 i = 1; i < n; ++i) {
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

static void blank_non_lucky (GameArraySimple& map, const PlayerState* seats, u16 seat_n) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = map.get_civ_owner(x, y);
            if (own == U8_KEY_NULL || own >= seat_n || seats[own].m_lucky == 0u) {
                map.set_civ_owner(x, y, U8_KEY_NULL);
            }
        }
    }
}

static u16 near_box_n (const CityArray& cities, u16 seat, u16 x, u16 y) {
    const i32 x0 = static_cast<i32>(x) - k_half;
    const i32 y0 = static_cast<i32>(y) - k_half;
    const i32 x1 = x0 + k_box;
    const i32 y1 = y0 + k_box;
    u16 n = 0u;
    const u16 cn = cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 ow = c->get_owner();
        if (ow == seat || ow == U16_KEY_NULL) {
            continue;
        }
        const i32 tx = static_cast<i32>(c->get_x());
        const i32 ty = static_cast<i32>(c->get_y());
        if (tx >= x0 && tx < x1 && ty >= y0 && ty < y1) {
            n = static_cast<u16>(n + 1u);
        }
    }
    return n;
}

static bool wr_muster_anno (
    cstr path,
    u32 turn,
    BorderTurnHandler& bth,
    const PlayerState* seats,
    u16 seat_n,
    const CityArray& cities) {
    if (path == nullptr || seats == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# border_turn_handler muster turn=%u\n", turn);
    std::fprintf(fp, "# fmt: x y text; @N = heat dot for 50x50 near targets (0..20)\n");
    u16 wrote = 0u;
    for (u16 seat = 0; seat < seat_n; ++seat) {
        if (seats[seat].m_lucky == 0u) {
            continue;
        }
        static const u16 k_n = 4u;
        BorderFront fronts[BorderTurnHandler::k_front_cap];
        for (u16 i = 0; i < k_n; ++i) {
            fronts[i].m_city = BorderTurnHandler::k_front_need;
            fronts[i].m_pl = U16_KEY_NULL;
        }
        if (!bth.handle(seat, fronts, k_n)) {
            continue;
        }
        const u16 cap = bth.capital_city();
        if (cap != U16_KEY_NULL) {
            const City* c = cities.get_city(cap);
            if (c != nullptr) {
                const u16 x = c->get_x();
                const u16 y = c->get_y();
                std::fprintf(fp, "%u %u Cap %u\n",
                    static_cast<u32>(x), static_cast<u32>(y), static_cast<u32>(seat));
                std::fprintf(fp, "%u %u @%u\n",
                    static_cast<u32>(x), static_cast<u32>(y),
                    static_cast<u32>(near_box_n(cities, seat, x, y)));
                wrote = static_cast<u16>(wrote + 1u);
            }
        }
        for (u16 i = 0; i < bth.front_n(); ++i) {
            const u16 x = bth.front_x(i);
            const u16 y = bth.front_y(i);
            std::fprintf(fp, "%u %u @%u\n",
                static_cast<u32>(x), static_cast<u32>(y),
                static_cast<u32>(near_box_n(cities, seat, x, y)));
            wrote = static_cast<u16>(wrote + 1u);
        }
    }
    std::fclose(fp);
    return wrote > 0u;
}

static bool run_annotate (cstr img, cstr anno, cstr out) {
    char cmd[1536];
    if (std::snprintf(cmd, sizeof(cmd), "python3 \"%s\" \"%s\" \"%s\" \"%s\"",
            G_PLOT, img, anno, out) <= 0) {
        return false;
    }
    return std::system(cmd) == 0;
}

//================================================================================================================================
//=> - BorderTurnHandlerTester -
//================================================================================================================================

class BorderTurnHandlerTester : public EvalDriver {
public:
    BorderTurnHandlerTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

BorderTurnHandlerTester::BorderTurnHandlerTester ()
    : EvalDriver(make_need()) {
}

int BorderTurnHandlerTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n == 0u) {
        std::printf("border_turn_handler: no saves\n");
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("border_turn_handler: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("border_turn_handler: TechAgeMng::setup failed\n");
        return 1;
    }
    const char* mk = std::getenv("BORDER_MK");
    if (mk == nullptr || mk[0] == 0) {
        mk = "mk02";
    }
    char data_name[128];
    if (std::snprintf(data_name, sizeof(data_name), "border-turn-handler-%s", mk) <= 0) {
        std::printf("border_turn_handler: bad data name\n");
        TechAgeMng::clear();
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(data_name, data_dir, sizeof(data_dir))) {
        std::printf("border_turn_handler: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    char out_dir[512];
    if (std::snprintf(out_dir, sizeof(out_dir), "%s/%s", eval_dir, data_name) <= 0) {
        std::printf("border_turn_handler: bad out_dir\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(out_dir) || !mkdir_deep(data_dir)) {
        std::printf("border_turn_handler: cannot mkdir\n");
        TechAgeMng::clear();
        return 1;
    }
    for (u16 si = 0; si < save_n; ++si) {
        const u32 turn = paths().save_turn_at(si);
        char map_p[512];
        char cities_p[512];
        char players_p[512];
        if (!paths().map_path(turn, map_p, sizeof(map_p))
            || !paths().cities_path(turn, cities_p, sizeof(cities_p))
            || !paths().players_path(turn, players_p, sizeof(players_p))) {
            std::printf("border_turn_handler: bad paths turn=%u\n", turn);
            TechAgeMng::clear();
            return 1;
        }
        GameArraySimple map;
        if (!GameIo::load_map_tiles(map_p, map)) {
            std::printf("border_turn_handler: load_map failed %s\n", map_p);
            TechAgeMng::clear();
            return 1;
        }
        CityArray cities;
        if (!GameIo::load_cities(cities_p, cities)) {
            std::printf("border_turn_handler: load_cities failed %s\n", cities_p);
            TechAgeMng::clear();
            return 1;
        }
        PlayerState* seats = nullptr;
        u16 seat_n = 0;
        if (!GameIo::load_players(players_p, seats, seat_n)) {
            std::printf("border_turn_handler: load_players failed %s\n", players_p);
            TechAgeMng::clear();
            return 1;
        }
        BorderTurnHandler bth;
        if (!bth.begin(map, cities)) {
            std::printf("border_turn_handler: begin failed turn=%u\n", turn);
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        u16 lucky_n = 0u;
        u16 ok_n = 0u;
        const auto t0 = std::chrono::steady_clock::now();
        for (u16 seat = 0; seat < seat_n; ++seat) {
            if (seats[seat].m_lucky == 0u) {
                continue;
            }
            lucky_n = static_cast<u16>(lucky_n + 1u);
            static const u16 k_n = 4u;
            BorderFront fronts[BorderTurnHandler::k_front_cap];
            for (u16 i = 0; i < k_n; ++i) {
                fronts[i].m_city = BorderTurnHandler::k_front_need;
                fronts[i].m_pl = U16_KEY_NULL;
            }
            if (bth.handle(seat, fronts, k_n)) {
                ok_n = static_cast<u16>(ok_n + 1u);
            }
        }
        const auto t1 = std::chrono::steady_clock::now();
        const double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        std::printf("border_turn_handler turn=%u lucky=%u ok=%u handle_ms=%.3f\n",
            turn, static_cast<u32>(lucky_n), static_cast<u32>(ok_n), ms);

        blank_non_lucky(map, seats, seat_n);
        char img_p[512];
        char anno_p[512];
        char out_p[512];
        if (std::snprintf(img_p, sizeof(img_p), "%s/border_lucky_t%04u.ppm", out_dir, turn) <= 0
            || std::snprintf(anno_p, sizeof(anno_p), "%s/anno_muster_t%04u.txt", data_dir, turn) <= 0
            || std::snprintf(out_p, sizeof(out_p), "%s/border_muster_t%04u.png", out_dir, turn) <= 0) {
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        if (!MapOwnSaveSnap::write(img_p, map)) {
            std::printf("border_turn_handler: map write failed turn=%u\n", turn);
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        if (!wr_muster_anno(anno_p, turn, bth, seats, seat_n, cities)) {
            std::printf("border_turn_handler: no muster anno turn=%u\n", turn);
            free_seats(seats, seat_n);
            continue;
        }
        free_seats(seats, seat_n);
        std::fflush(stdout);
        if (!run_annotate(img_p, anno_p, out_p)) {
            std::printf("border_turn_handler: annotate failed turn=%u\n", turn);
            TechAgeMng::clear();
            return 1;
        }
        std::printf("wrote %s\n", out_p);
    }
    TechAgeMng::clear();
    if (WhiteboardMng::chkout() == 0u
        && (WhiteboardMng::width() != 0u || WhiteboardMng::height() != 0u)) {
        WhiteboardMng::terminate();
    }
    std::printf("border_turn_handler ok saves=%u\n", static_cast<u32>(save_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    BorderTurnHandlerTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
