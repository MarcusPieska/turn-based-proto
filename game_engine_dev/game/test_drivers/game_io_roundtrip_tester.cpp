//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include <dirent.h>

#include "city_array.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_io_cmp.h"
#include "game_state.h"
#include "unit_add_vector.h"

//================================================================================================================================
//=> - Paths -
//================================================================================================================================

static const char* G_SAVES = "/home/w/Projects/game-saves";
static const char* G_MAP_B = "/tmp/game_io_rt_map.bin";
static const char* G_UNITS_B = "/tmp/game_io_rt_units.bin";
static const char* G_CITIES_B = "/tmp/game_io_rt_cities.bin";
static const char* G_PLAYERS_B = "/tmp/game_io_rt_players.bin";

//================================================================================================================================
//=> - Save pick -
//================================================================================================================================

struct SavePick {
    char m_map[512];
    char m_units[512];
    char m_cities[512];
    char m_players[512];
    u32 m_seed;
    u32 m_turn;
    u16 m_pn;
};

static bool file_ok (cstr path) {
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

static bool better (u32 turn, u32 seed, u16 pn, const SavePick& cur) {
    if (turn != cur.m_turn) {
        return turn > cur.m_turn;
    }
    if (seed != cur.m_seed) {
        return seed > cur.m_seed;
    }
    return pn > cur.m_pn;
}

static bool pick_latest (SavePick* out) {
    if (out == nullptr) {
        return false;
    }
    DIR* d = ::opendir(G_SAVES);
    if (d == nullptr) {
        std::printf("WARN: cannot open save folder '%s'; aborting game_io roundtrip\n", G_SAVES);
        return false;
    }
    bool found = false;
    SavePick best = {};
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
        char expect[256];
        if (std::snprintf(expect, sizeof(expect), "game-loop-seed-%u-p%u-%04u.bin", seed, pn, turn) <= 0
            || std::strcmp(e->d_name, expect) != 0) {
            continue;
        }
        SavePick cand = {};
        cand.m_seed = seed;
        cand.m_turn = turn;
        cand.m_pn = static_cast<u16>(pn);
        if (std::snprintf(cand.m_map, sizeof(cand.m_map), "%s/%s", G_SAVES, expect) <= 0
            || std::snprintf(cand.m_units, sizeof(cand.m_units),
                "%s/game-loop-seed-%u-p%u-%04u-units.bin", G_SAVES, seed, pn, turn) <= 0
            || std::snprintf(cand.m_cities, sizeof(cand.m_cities),
                "%s/game-loop-seed-%u-p%u-%04u-cities.bin", G_SAVES, seed, pn, turn) <= 0
            || std::snprintf(cand.m_players, sizeof(cand.m_players),
                "%s/game-loop-seed-%u-p%u-%04u-players.bin", G_SAVES, seed, pn, turn) <= 0) {
            continue;
        }
        if (!file_ok(cand.m_map) || !file_ok(cand.m_units)
            || !file_ok(cand.m_cities) || !file_ok(cand.m_players)) {
            continue;
        }
        if (!found || better(cand.m_turn, cand.m_seed, cand.m_pn, best)) {
            best = cand;
            found = true;
        }
    }
    ::closedir(d);
    if (!found) {
        std::printf("WARN: no complete game-loop save (map+units+cities+players) in '%s'; aborting\n",
            G_SAVES);
        return false;
    }
    *out = best;
    return true;
}

//================================================================================================================================
//=> - Main -
//================================================================================================================================

int main () {
    SavePick src = {};
    if (!pick_latest(&src)) {
        return 1;
    }
    std::printf("game_io roundtrip using seed=%u players=%u turn=%u\n",
        src.m_seed, static_cast<u32>(src.m_pn), src.m_turn);

    GameArraySimple map_a;
    GameArraySimple map_b;
    UnitAddVector units_a;
    UnitAddVector units_b;
    CityArray cities_a;
    CityArray cities_b;
    PlayerState* seats_a = nullptr;
    PlayerState* seats_b = nullptr;
    u16 seat_n_a = 0;
    u16 seat_n_b = 0;

    if (!GameIo::load_map_tiles(src.m_map, map_a)
        || !GameIo::load_units(src.m_units, units_a)
        || !GameIo::load_cities(src.m_cities, cities_a)
        || !GameIo::load_players(src.m_players, seats_a, seat_n_a)) {
        std::printf("load A failed\n");
        return 1;
    }
    if (!GameIo::save_map_tiles(G_MAP_B, map_a)
        || !GameIo::save_units(G_UNITS_B, units_a)
        || !GameIo::save_cities(G_CITIES_B, cities_a)
        || !GameIo::save_players(G_PLAYERS_B, seats_a, seat_n_a)) {
        std::printf("save B failed\n");
        return 1;
    }
    if (!GameIo::load_map_tiles(G_MAP_B, map_b)
        || !GameIo::load_units(G_UNITS_B, units_b)
        || !GameIo::load_cities(G_CITIES_B, cities_b)
        || !GameIo::load_players(G_PLAYERS_B, seats_b, seat_n_b)) {
        std::printf("load B failed\n");
        return 1;
    }
    if (!GameIoCmp::map(map_a, map_b)
        || !GameIoCmp::units(units_a, units_b)
        || !GameIoCmp::cities(cities_a, cities_b)
        || !GameIoCmp::players(seats_a, seat_n_a, seats_b, seat_n_b)) {
        return 1;
    }
    std::printf("game_io roundtrip ok map=%ux%u units_head=%u cities=%u players=%u\n",
        map_a.width(), map_a.height(),
        static_cast<u32>(units_a.get_head_unit_add_idx()),
        static_cast<u32>(cities_a.get_city_count()),
        static_cast<u32>(seat_n_a));
    for (u16 i = 0; i < seat_n_a; ++i) {
        delete seats_a[i].m_techs_researched;
    }
    delete[] seats_a;
    for (u16 i = 0; i < seat_n_b; ++i) {
        delete seats_b[i].m_techs_researched;
    }
    delete[] seats_b;
    return 0;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
