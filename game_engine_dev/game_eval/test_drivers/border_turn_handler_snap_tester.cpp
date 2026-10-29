//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
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
static const char* G_OUT_NAME = "border-turn-handler-snap";
static const u32 k_turn = 500u;
static const u16 k_slot_n = BorderTurnHandler::k_front_cap;
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

static void blank_except (GameArraySimple& map, u16 seat) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_civ_owner(x, y) != static_cast<u8>(seat)) {
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

static bool wr_seat_anno (
    cstr path,
    u16 seat,
    BorderTurnHandler& bth,
    const CityArray& cities) {
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# border snap seat=%u\n", static_cast<u32>(seat));
    u16 wrote = 0u;
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

static void set_all_need (BorderFront* fronts, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        fronts[i].m_city = BorderTurnHandler::k_front_need;
        fronts[i].m_pl = U16_KEY_NULL;
    }
}

static bool fill_all (BorderTurnHandler& bth, u16 seat, BorderFront* fronts, u16 n) {
    set_all_need(fronts, n);
    return bth.handle(seat, fronts, n);
}

static bool fill_one_at_a_time (BorderTurnHandler& bth, u16 seat, BorderFront* fronts, u16 n) {
    for (u16 i = 0; i < n; ++i) {
        fronts[i].m_city = BorderTurnHandler::k_front_idle;
        fronts[i].m_pl = U16_KEY_NULL;
    }
    for (u16 p = 0; p < n; ++p) {
        fronts[p].m_city = BorderTurnHandler::k_front_need;
        fronts[p].m_pl = U16_KEY_NULL;
        if (!bth.handle(seat, fronts, n)) {
            return false;
        }
        if (BorderTurnHandler::slot_need(fronts[p].m_city)) {
            break;
        }
    }
    return true;
}

static bool snap_one (
    cstr map_p,
    const CityArray& cities,
    u16 seat,
    BorderTurnHandler& bth,
    cstr mk,
    cstr mode,
    cstr out_dir,
    cstr data_dir,
    bool (*fill)(BorderTurnHandler&, u16, BorderFront*, u16)) {
    GameArraySimple map;
    if (!GameIo::load_map_tiles(map_p, map)) {
        return false;
    }
    if (!bth.begin(map, cities)) {
        return false;
    }
    BorderFront fronts[BorderTurnHandler::k_front_cap];
    if (!fill(bth, seat, fronts, k_slot_n)) {
        return false;
    }
    blank_except(map, seat);
    char img_p[512];
    char anno_p[512];
    char out_p[512];
    if (std::snprintf(img_p, sizeof(img_p), "%s/seat%03u-%s-mode%s.ppm",
            data_dir, static_cast<u32>(seat), mk, mode) <= 0
        || std::snprintf(anno_p, sizeof(anno_p), "%s/seat%03u-%s-mode%s.txt",
            data_dir, static_cast<u32>(seat), mk, mode) <= 0
        || std::snprintf(out_p, sizeof(out_p), "%s/seat%03u-%s-mode%s.png",
            out_dir, static_cast<u32>(seat), mk, mode) <= 0) {
        return false;
    }
    if (!MapOwnSaveSnap::write(img_p, map)) {
        return false;
    }
    if (!wr_seat_anno(anno_p, seat, bth, cities)) {
        return false;
    }
    std::fflush(stdout);
    if (!run_annotate(img_p, anno_p, out_p)) {
        return false;
    }
    std::printf("wrote %s fronts=%u\n", out_p, static_cast<u32>(bth.front_n()));
    return true;
}

//================================================================================================================================
//=> - BorderTurnHandlerSnapTester -
//================================================================================================================================

class BorderTurnHandlerSnapTester : public EvalDriver {
public:
    BorderTurnHandlerSnapTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

BorderTurnHandlerSnapTester::BorderTurnHandlerSnapTester ()
    : EvalDriver(make_need()) {
}

int BorderTurnHandlerSnapTester::run () {
    const u16 save_n = paths().save_turn_n();
    bool have = false;
    for (u16 si = 0; si < save_n; ++si) {
        if (paths().save_turn_at(si) == k_turn) {
            have = true;
            break;
        }
    }
    if (!have) {
        std::printf("border_snap: turn %u not in save seq\n", k_turn);
        return 1;
    }
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("border_snap: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("border_snap: TechAgeMng::setup failed\n");
        return 1;
    }
    const char* mk = std::getenv("BORDER_MK");
    if (mk == nullptr || mk[0] == 0) {
        mk = "mk06";
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_OUT_NAME, data_dir, sizeof(data_dir))) {
        std::printf("border_snap: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    char out_dir[512];
    if (std::snprintf(out_dir, sizeof(out_dir), "%s/%s", eval_dir, G_OUT_NAME) <= 0) {
        std::printf("border_snap: bad out_dir\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(out_dir) || !mkdir_deep(data_dir)) {
        std::printf("border_snap: cannot mkdir\n");
        TechAgeMng::clear();
        return 1;
    }
    char map_p[512];
    char cities_p[512];
    char players_p[512];
    if (!paths().map_path(k_turn, map_p, sizeof(map_p))
        || !paths().cities_path(k_turn, cities_p, sizeof(cities_p))
        || !paths().players_path(k_turn, players_p, sizeof(players_p))) {
        std::printf("border_snap: bad paths turn=%u\n", k_turn);
        TechAgeMng::clear();
        return 1;
    }
    CityArray cities;
    if (!GameIo::load_cities(cities_p, cities)) {
        std::printf("border_snap: load_cities failed\n");
        TechAgeMng::clear();
        return 1;
    }
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n)) {
        std::printf("border_snap: load_players failed\n");
        TechAgeMng::clear();
        return 1;
    }
    u16 done = 0u;
    for (u16 seat = 0; seat < seat_n; ++seat) {
        if (seats[seat].m_lucky == 0u) {
            continue;
        }
        BorderTurnHandler bth;
        if (!snap_one(map_p, cities, seat, bth, mk, "all", out_dir, data_dir, fill_all)) {
            std::printf("border_snap: all failed seat=%u mk=%s\n", static_cast<u32>(seat), mk);
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        if (!snap_one(map_p, cities, seat, bth, mk, "one", out_dir, data_dir, fill_one_at_a_time)) {
            std::printf("border_snap: one failed seat=%u mk=%s\n", static_cast<u32>(seat), mk);
            free_seats(seats, seat_n);
            TechAgeMng::clear();
            return 1;
        }
        done = static_cast<u16>(done + 1u);
    }
    free_seats(seats, seat_n);
    TechAgeMng::clear();
    if (WhiteboardMng::chkout() == 0u
        && (WhiteboardMng::width() != 0u || WhiteboardMng::height() != 0u)) {
        WhiteboardMng::terminate();
    }
    std::printf("border_snap ok turn=%u mk=%s seats=%u\n",
        k_turn, mk, static_cast<u32>(done));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    BorderTurnHandlerSnapTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
