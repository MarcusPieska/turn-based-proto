//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_seat_save_ledger_mng.h"

#include <cstdio>
#include <cstring>

#include "city.h"
#include "city_array.h"
#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tech_age_mng.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

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

//================================================================================================================================
//=> - LuckySeatSaveLedgerMng -
//================================================================================================================================

LuckySeatSaveLedgerMng::LuckySeatSaveLedgerMng ()
    : m_lucky(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0) {
}

LuckySeatSaveLedgerMng::~LuckySeatSaveLedgerMng () {
    clr();
}

void LuckySeatSaveLedgerMng::clr () {
    delete[] m_lucky;
    m_lucky = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
}

bool LuckySeatSaveLedgerMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0 || save_n < 2) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    return true;
}

bool LuckySeatSaveLedgerMng::fill (const EvalPaths& paths, cstr out_dir, const RuntimeStatics& st) {
    if (out_dir == nullptr || out_dir[0] == 0 || !paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    if (!TechAgeMng::ready()) {
        return false;
    }
    const u16 typ_n = st.unit().get_item_count();
    if (typ_n == 0) {
        return false;
    }
    char players_p[512];
    if (!paths.players_path(paths.save_turn_at(0), players_p, sizeof(players_p))) {
        return false;
    }
    PlayerState* seats = nullptr;
    u16 seat_n = 0;
    if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
        free_seats(seats, seat_n);
        return false;
    }
    u16 lucky_cap = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            lucky_cap = static_cast<u16>(lucky_cap + 1u);
        }
    }
    if (lucky_cap == 0) {
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky = new u16[lucky_cap];
    m_lucky_n = 0;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            m_lucky[m_lucky_n] = p;
            m_lucky_n = static_cast<u16>(m_lucky_n + 1u);
        }
    }
    free_seats(seats, seat_n);

    std::FILE** fps = new std::FILE*[m_lucky_n];
    for (u16 i = 0; i < m_lucky_n; ++i) {
        fps[i] = nullptr;
    }
    for (u16 i = 0; i < m_lucky_n; ++i) {
        char path[512];
        if (std::snprintf(path, sizeof(path), "%s/lucky_seat_%03u.txt",
                out_dir, static_cast<u32>(m_lucky[i])) <= 0) {
            for (u16 j = 0; j < i; ++j) {
                std::fclose(fps[j]);
            }
            delete[] fps;
            return false;
        }
        fps[i] = std::fopen(path, "w");
        if (fps[i] == nullptr) {
            for (u16 j = 0; j < i; ++j) {
                std::fclose(fps[j]);
            }
            delete[] fps;
            return false;
        }
        std::fprintf(fps[i], "# seat=%u lucky=1\n", static_cast<u32>(m_lucky[i]));
        std::printf("opened %s\n", path);
    }

    u32* tallies = new u32[typ_n];
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        char cities_p[512];
        if (!paths.players_path(turn, players_p, sizeof(players_p))
            || !paths.units_path(turn, units_p, sizeof(units_p))
            || !paths.cities_path(turn, cities_p, sizeof(cities_p))) {
            delete[] tallies;
            for (u16 i = 0; i < m_lucky_n; ++i) {
                std::fclose(fps[i]);
            }
            delete[] fps;
            return false;
        }
        seats = nullptr;
        seat_n = 0;
        if (!GameIo::load_players(players_p, seats, seat_n) || seat_n < m_player_n) {
            free_seats(seats, seat_n);
            delete[] tallies;
            for (u16 i = 0; i < m_lucky_n; ++i) {
                std::fclose(fps[i]);
            }
            delete[] fps;
            return false;
        }
        UnitAddVector units;
        CityArray cities;
        if (!GameIo::load_units(units_p, units) || !GameIo::load_cities(cities_p, cities)) {
            free_seats(seats, seat_n);
            delete[] tallies;
            for (u16 i = 0; i < m_lucky_n; ++i) {
                std::fclose(fps[i]);
            }
            delete[] fps;
            return false;
        }
        for (u16 li = 0; li < m_lucky_n; ++li) {
            const u16 seat = m_lucky[li];
            for (u16 t = 0; t < typ_n; ++t) {
                tallies[t] = 0;
            }
            u32 unit_tot = 0;
            const u16 head = units.get_head_unit_add_idx();
            for (u16 k = 0; k < head; ++k) {
                const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
                if (u == nullptr || u->m_x == U16_KEY_NULL) {
                    continue;
                }
                if (u->m_player_idx != seat) {
                    continue;
                }
                const u16 typ = static_cast<u16>(u->m_unit_typ_idx);
                if (typ >= typ_n) {
                    continue;
                }
                tallies[typ] = tallies[typ] + 1u;
                unit_tot = unit_tot + 1u;
            }
            u32 city_tot = 0;
            const u16 cn = cities.get_city_count();
            for (u16 ci = 0; ci < cn; ++ci) {
                const City* c = cities.get_city(ci);
                if (c == nullptr || c->get_owner() != seat) {
                    continue;
                }
                city_tot = city_tot + 1u;
            }
            const u32 commerce = seats[seat].m_commerce;
            std::fprintf(fps[li], "--- turn %u ---\n", turn);
            std::fprintf(fps[li], "units=%u commerce=%u cities=%u\n",
                unit_tot, commerce, city_tot);
            for (u16 t = 0; t < typ_n; ++t) {
                if (tallies[t] == 0u) {
                    continue;
                }
                cstr nm = st.unit().get_name(UnitStaticDataKey::from_raw(t));
                if (nm == nullptr) {
                    nm = "?";
                }
                std::fprintf(fps[li], "%s %u\n", nm, tallies[t]);
            }
            std::fprintf(fps[li], "\n");
        }
        free_seats(seats, seat_n);
    }
    delete[] tallies;
    for (u16 i = 0; i < m_lucky_n; ++i) {
        std::fclose(fps[i]);
    }
    delete[] fps;
    return true;
}

u16 LuckySeatSaveLedgerMng::save_n () const {
    return m_save_n;
}

u16 LuckySeatSaveLedgerMng::lucky_n () const {
    return m_lucky_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
