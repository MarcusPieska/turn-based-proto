//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_bin.h"

#include <cstdio>

#include "city_array.h"
#include "eval_need.h"
#include "eval_paths.h"
#include "game_array_simple.h"
#include "game_io.h"
#include "game_state.h"
#include "unit_add_vector.h"

//================================================================================================================================
//=> - Snap -
//================================================================================================================================

struct EvalBin::Snap {
    GameArraySimple m_map;
    UnitAddVector m_units;
    CityArray m_cities;
    PlayerState* m_seats;
    u16 m_seat_n;
    u32 m_turn;

    Snap () : m_seats(nullptr), m_seat_n(0), m_turn(0) {
    }

    ~Snap () {
        if (m_seats != nullptr) {
            for (u16 i = 0; i < m_seat_n; ++i) {
                delete m_seats[i].m_techs_researched;
                m_seats[i].m_techs_researched = nullptr;
            }
            delete[] m_seats;
            m_seats = nullptr;
            m_seat_n = 0;
        }
    }
};

//================================================================================================================================
//=> - EvalBin -
//================================================================================================================================

EvalBin::EvalBin () : m_snaps(nullptr), m_n(0), m_cap(0) {
}

EvalBin::~EvalBin () {
    clr();
}

void EvalBin::clr () {
    if (m_snaps != nullptr) {
        for (u16 i = 0; i < m_n; ++i) {
            delete m_snaps[i];
            m_snaps[i] = nullptr;
        }
        delete[] m_snaps;
        m_snaps = nullptr;
    }
    m_n = 0;
    m_cap = 0;
}

bool EvalBin::file_ok (cstr path) const {
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "rb");
    if (fp == nullptr) {
        return false;
    }
    std::fclose(fp);
    return true;
}

bool EvalBin::load_turn (const EvalPaths& paths, u32 turn) {
    if (!paths.ok() || turn == 0) {
        return false;
    }
    for (u16 i = 0; i < m_n; ++i) {
        if (m_snaps[i] != nullptr && m_snaps[i]->m_turn == turn) {
            return true;
        }
    }
    char map_p[512];
    char units_p[512];
    char cities_p[512];
    char players_p[512];
    if (!paths.map_path(turn, map_p, sizeof(map_p))
        || !paths.units_path(turn, units_p, sizeof(units_p))
        || !paths.cities_path(turn, cities_p, sizeof(cities_p))
        || !paths.players_path(turn, players_p, sizeof(players_p))) {
        return false;
    }
    if (!file_ok(map_p) || !file_ok(units_p) || !file_ok(cities_p) || !file_ok(players_p)) {
        return false;
    }
    if (m_snaps == nullptr) {
        m_cap = EvalNeed::SAVE_MAX;
        m_snaps = new Snap*[m_cap];
        for (u16 i = 0; i < m_cap; ++i) {
            m_snaps[i] = nullptr;
        }
    }
    if (m_n >= m_cap) {
        return false;
    }
    Snap* s = new Snap();
    s->m_turn = turn;
    if (!GameIo::load_map_tiles(map_p, s->m_map)
        || !GameIo::load_units(units_p, s->m_units)
        || !GameIo::load_cities(cities_p, s->m_cities)
        || !GameIo::load_players(players_p, s->m_seats, s->m_seat_n)) {
        delete s;
        return false;
    }
    m_snaps[m_n] = s;
    m_n = static_cast<u16>(m_n + 1u);
    return true;
}

u16 EvalBin::snap_n () const {
    return m_n;
}

u32 EvalBin::turn_at (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return 0;
    }
    return m_snaps[i]->m_turn;
}

u16 EvalBin::snap_i (u32 turn) const {
    for (u16 i = 0; i < m_n; ++i) {
        if (m_snaps[i] != nullptr && m_snaps[i]->m_turn == turn) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

const GameArraySimple* EvalBin::map (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return nullptr;
    }
    return &m_snaps[i]->m_map;
}

const UnitAddVector* EvalBin::units (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return nullptr;
    }
    return &m_snaps[i]->m_units;
}

const CityArray* EvalBin::cities (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return nullptr;
    }
    return &m_snaps[i]->m_cities;
}

const PlayerState* EvalBin::seats (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return nullptr;
    }
    return m_snaps[i]->m_seats;
}

u16 EvalBin::seat_n (u16 i) const {
    if (i >= m_n || m_snaps == nullptr || m_snaps[i] == nullptr) {
        return 0;
    }
    return m_snaps[i]->m_seat_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
