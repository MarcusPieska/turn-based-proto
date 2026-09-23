//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include <dirent.h>

#include "city_array.h"
#include "eval_driver.h"
#include "eval_need.h"
#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_state.h"
#include "unit_add_vector.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_PATHS = "/home/w/Projects/rts-proto/game_engine_dev/game_eval/eval_paths.txt";

//================================================================================================================================
//=> - Latest turn -
//================================================================================================================================

static bool file_ok (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static u32 latest_turn (const EvalPaths& paths) {
    DIR* d = ::opendir(paths.saves_root());
    if (d == nullptr) {
        return 0;
    }
    u32 best = 0;
    char expect[256];
    char map_p[512];
    char units_p[512];
    char cities_p[512];
    char players_p[512];
    while (const dirent* e = ::readdir(d)) {
        if (e->d_name[0] == '.') {
            continue;
        }
        unsigned seed = 0;
        unsigned pn = 0;
        unsigned turn = 0;
        if (std::sscanf(e->d_name, "game-loop-seed-%u-p%u-%u.bin", &seed, &pn, &turn) != 3) {
            continue;
        }
        if (seed != paths.seed() || pn != paths.players()) {
            continue;
        }
        if (std::snprintf(expect, sizeof(expect), "game-loop-seed-%u-p%u-%04u.bin", seed, pn, turn) <= 0
            || std::strcmp(e->d_name, expect) != 0) {
            continue;
        }
        if (!paths.map_path(turn, map_p, sizeof(map_p))
            || !paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.cities_path(turn, cities_p, sizeof(cities_p))
            || !paths.players_path(turn, players_p, sizeof(players_p))) {
            continue;
        }
        if (!file_ok(map_p) || !file_ok(units_p) || !file_ok(cities_p) || !file_ok(players_p)) {
            continue;
        }
        if (turn > best) {
            best = turn;
        }
    }
    ::closedir(d);
    return best;
}

//================================================================================================================================
//=> - SaveUnpackTester -
//================================================================================================================================

class SaveUnpackTester : public EvalDriver {
public:
    explicit SaveUnpackTester (u32 turn);

protected:
    int run () override;

private:
    u32 m_turn;
};

static EvalNeed make_need (u32 turn) {
    EvalNeed n;
    n.save(turn);
    return n;
}

SaveUnpackTester::SaveUnpackTester (u32 turn)
    : EvalDriver(make_need(turn)),
      m_turn(turn) {
}

int SaveUnpackTester::run () {
    const u16 i = bin().snap_i(m_turn);
    if (i == U16_KEY_NULL) {
        std::printf("save_unpack: missing snap for turn=%u\n", m_turn);
        return 1;
    }
    const GameArraySimple* map = bin().map(i);
    const UnitAddVector* units = bin().units(i);
    const CityArray* cities = bin().cities(i);
    const PlayerState* seats = bin().seats(i);
    const u16 seat_n = bin().seat_n(i);
    if (map == nullptr || units == nullptr || cities == nullptr || seats == nullptr || seat_n == 0) {
        std::printf("save_unpack: null snap members turn=%u\n", m_turn);
        return 1;
    }
    std::printf("save_unpack ok turn=%u map=%ux%u units_head=%u cities=%u players=%u\n",
        m_turn,
        map->width(), map->height(),
        static_cast<u32>(units->get_head_unit_add_idx()),
        static_cast<u32>(cities->get_city_count()),
        static_cast<u32>(seat_n));
    return 0;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    EvalPaths paths;
    if (!paths.load(G_PATHS)) {
        return 1;
    }
    const u32 turn = latest_turn(paths);
    if (turn == 0) {
        std::printf("WARN: no complete save quartet for seed=%u players=%u under '%s'; aborting\n",
            paths.seed(), static_cast<u32>(paths.players()), paths.saves_root());
        return 1;
    }
    std::printf("save_unpack using seed=%u players=%u turn=%u\n", paths.seed(), static_cast<u32>(paths.players()), turn);
    SaveUnpackTester t(turn);
    return t.go(G_PATHS);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
