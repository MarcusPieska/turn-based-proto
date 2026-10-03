//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

#include "eval_driver.h"
#include "eval_need.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_map_defs.h"
#include "game_state.h"
#include "map_own_save_snap.h"
#include "runtime_static_loader.h"
#include "tech_age_mng.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";
static const char* G_DATA_NAME = "basic-game-state-info";
static const char* G_PLOT = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/test_drivers/annotate_map.py";
static const char* G_RT_LIB = "/home/w/Projects/rts-proto/game_engine_dev/data_io/runtime_static_loader_lib.so";
static const char* G_RT_DATA = "/home/w/Projects/rts-proto/game_engine_dev/";
static const u16 k_seat_cap = 256u;

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

static bool is_open_water (u8 terr) {
    return terr == TERR_OCEAN[0] || terr == TERR_SEA[0] || terr == TERR_COASTAL[0];
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
        delete seats[i].m_tech_age;
        seats[i].m_tech_age = nullptr;
        seats[i].m_res_ledger.clear();
    }
    delete[] seats;
    seats = nullptr;
    n = 0;
}

static void fill_centroids (const GameArraySimple& map, u16* cx, u16* cy, u32* sn) {
    u32 sx[k_seat_cap];
    u32 sy[k_seat_cap];
    for (u16 i = 0; i < k_seat_cap; ++i) {
        sx[i] = 0u;
        sy[i] = 0u;
        sn[i] = 0u;
        cx[i] = 0u;
        cy[i] = 0u;
    }
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            const u8 own = map.get_civ_owner(x, y);
            if (own == U8_KEY_NULL || own >= k_seat_cap) {
                continue;
            }
            if (is_open_water(map.get_terrain(x, y))) {
                continue;
            }
            sx[own] += x;
            sy[own] += y;
            sn[own] += 1u;
        }
    }
    for (u16 seat = 0; seat < k_seat_cap; ++seat) {
        if (sn[seat] == 0u) {
            continue;
        }
        cx[seat] = static_cast<u16>(sx[seat] / sn[seat]);
        cy[seat] = static_cast<u16>(sy[seat] / sn[seat]);
    }
}

static bool wr_seat_anno (cstr path, u32 turn, const u16* cx, const u16* cy, const u32* sn) {
    if (path == nullptr || cx == nullptr || cy == nullptr || sn == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# basic_game_state_info seats turn=%u\n", turn);
    std::fprintf(fp, "# fmt: x y text\n");
    u16 wrote = 0u;
    for (u16 seat = 0; seat < k_seat_cap; ++seat) {
        if (sn[seat] == 0u) {
            continue;
        }
        std::fprintf(fp, "%u %u %u\n", static_cast<u32>(cx[seat]), static_cast<u32>(cy[seat]),
            static_cast<u32>(seat));
        wrote = static_cast<u16>(wrote + 1u);
    }
    std::fclose(fp);
    return wrote > 0u;
}

static bool wr_lucky_anno (
    cstr path,
    u32 turn,
    const PlayerState* seats,
    u16 seat_n,
    const u16* cx,
    const u16* cy,
    const u32* sn) {
    if (path == nullptr || seats == nullptr || cx == nullptr || cy == nullptr || sn == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "w");
    if (fp == nullptr) {
        return false;
    }
    std::fprintf(fp, "# basic_game_state_info lucky turn=%u\n", turn);
    std::fprintf(fp, "# fmt: x y text\n");
    u16 wrote = 0u;
    const u16 n = (seat_n < k_seat_cap) ? seat_n : k_seat_cap;
    for (u16 seat = 0; seat < n; ++seat) {
        if (seats[seat].m_lucky == 0u || sn[seat] == 0u) {
            continue;
        }
        std::fprintf(fp, "%u %u Lucky: %u\n", static_cast<u32>(cx[seat]), static_cast<u32>(cy[seat]),
            static_cast<u32>(seat));
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

//================================================================================================================================
//=> - BasicGameStateInfoTester -
//================================================================================================================================

class BasicGameStateInfoTester : public EvalDriver {
public:
    BasicGameStateInfoTester ();

protected:
    int run () override;
};

static EvalNeed make_need () {
    EvalNeed n;
    n.save_seq();
    return n;
}

BasicGameStateInfoTester::BasicGameStateInfoTester ()
    : EvalDriver(make_need()) {
}

int BasicGameStateInfoTester::run () {
    const u16 save_n = paths().save_turn_n();
    if (save_n < 2) {
        std::printf("basic_game_state_info: bad sizes saves=%u\n", static_cast<u32>(save_n));
        return 1;
    }
    const u32 turn = paths().save_turn_at(static_cast<u16>(save_n - 1u));
    RuntimeStaticLoader loader;
    if (!loader.load(G_RT_LIB, G_RT_DATA)) {
        std::printf("basic_game_state_info: cannot load runtime statics\n");
        return 1;
    }
    if (!TechAgeMng::setup(loader.statics())) {
        std::printf("basic_game_state_info: TechAgeMng::setup failed\n");
        return 1;
    }
    char eval_dir[512];
    char data_dir[512];
    if (!paths().eval_dir(eval_dir, sizeof(eval_dir))
        || !paths().data_dir(G_DATA_NAME, data_dir, sizeof(data_dir))) {
        std::printf("basic_game_state_info: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!mkdir_deep(eval_dir) || !mkdir_deep(data_dir)) {
        std::printf("basic_game_state_info: cannot mkdir under %s\n", paths().out_root());
        TechAgeMng::clear();
        return 1;
    }
    char map_p[512];
    char players_p[512];
    if (!paths().map_path(turn, map_p, sizeof(map_p))
        || !paths().players_path(turn, players_p, sizeof(players_p))) {
        std::printf("basic_game_state_info: bad map/players path turn=%u\n", turn);
        TechAgeMng::clear();
        return 1;
    }
    char img_p[512];
    char anno_seats[512];
    char anno_lucky[512];
    char out_seats[512];
    char out_lucky[512];
    if (std::snprintf(img_p, sizeof(img_p), "%s/map_own_t%04u.ppm", eval_dir, turn) <= 0
        || std::snprintf(anno_seats, sizeof(anno_seats), "%s/anno_seats_t%04u.txt", data_dir, turn) <= 0
        || std::snprintf(anno_lucky, sizeof(anno_lucky), "%s/anno_lucky_t%04u.txt", data_dir, turn) <= 0
        || std::snprintf(out_seats, sizeof(out_seats), "%s/basic_info_seats_t%04u.png", eval_dir, turn) <= 0
        || std::snprintf(out_lucky, sizeof(out_lucky), "%s/basic_info_lucky_t%04u.png", eval_dir, turn) <= 0) {
        std::printf("basic_game_state_info: bad out paths\n");
        TechAgeMng::clear();
        return 1;
    }
    std::printf("basic_game_state_info writing final save turn=%u\n", turn);
    GameArraySimple map;
    if (!GameIo::load_map_tiles(map_p, map)) {
        std::printf("basic_game_state_info: load_map failed %s\n", map_p);
        TechAgeMng::clear();
        return 1;
    }
    if (!MapOwnSaveSnap::write(img_p, map)) {
        std::printf("basic_game_state_info: map write failed\n");
        TechAgeMng::clear();
        return 1;
    }
    std::printf("wrote %s\n", img_p);
    u16 cx[k_seat_cap];
    u16 cy[k_seat_cap];
    u32 sn[k_seat_cap];
    fill_centroids(map, cx, cy, sn);
    if (!wr_seat_anno(anno_seats, turn, cx, cy, sn)) {
        std::printf("basic_game_state_info: seat anno write failed\n");
        TechAgeMng::clear();
        return 1;
    }
    std::printf("wrote %s\n", anno_seats);
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n)) {
        std::printf("basic_game_state_info: load_players failed %s\n", players_p);
        TechAgeMng::clear();
        return 1;
    }
    if (!wr_lucky_anno(anno_lucky, turn, seats, seat_n, cx, cy, sn)) {
        std::printf("basic_game_state_info: lucky anno write failed\n");
        free_seats(seats, seat_n);
        TechAgeMng::clear();
        return 1;
    }
    free_seats(seats, seat_n);
    std::printf("wrote %s\n", anno_lucky);
    std::fflush(stdout);
    if (!run_annotate(img_p, anno_seats, out_seats)) {
        std::printf("basic_game_state_info: annotate seats failed\n");
        TechAgeMng::clear();
        return 1;
    }
    if (!run_annotate(img_p, anno_lucky, out_lucky)) {
        std::printf("basic_game_state_info: annotate lucky failed\n");
        TechAgeMng::clear();
        return 1;
    }
    TechAgeMng::clear();
    std::printf("basic_game_state_info ok turn=%u seats=%s lucky=%s\n", turn, out_seats, out_lucky);
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    BasicGameStateInfoTester t;
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
