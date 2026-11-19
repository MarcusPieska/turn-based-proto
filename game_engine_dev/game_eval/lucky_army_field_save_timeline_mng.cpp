//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_army_field_save_timeline_mng.h"

#include <cstdio>

#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tech_age_mng.h"
#include "unit_action_enum.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_add_vector_key.h"
#include "unit_static_key.h"
#include "unit_utility_helper.h"

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

static bool can_br (const RuntimeStatics& st, u16 typ_idx) {
    const u16 typ_n = st.unit().get_item_count();
    if (typ_idx >= typ_n) {
        return false;
    }
    const u16 ut = st.unit().get_item(UnitStaticDataKey::from_raw(typ_idx)).type;
    return st.unit_type_action_map().unit_type_can_do(ut, static_cast<u16>(UnitAction::canBarrage));
}

static u16 lucky_i (const u16* lucky, u16 lucky_n, u16 seat) {
    for (u16 i = 0; i < lucky_n; ++i) {
        if (lucky[i] == seat) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

//================================================================================================================================
//=> - LuckyArmyFieldSaveTimelineMng -
//================================================================================================================================

LuckyArmyFieldSaveTimelineMng::LuckyArmyFieldSaveTimelineMng ()
    : m_lucky(nullptr),
      m_turn(nullptr),
      m_tot(nullptr),
      m_tot_arty(nullptr),
      m_field(nullptr),
      m_field_arty(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0) {
}

LuckyArmyFieldSaveTimelineMng::~LuckyArmyFieldSaveTimelineMng () {
    clr();
}

void LuckyArmyFieldSaveTimelineMng::clr () {
    delete[] m_lucky;
    m_lucky = nullptr;
    delete[] m_turn;
    m_turn = nullptr;
    delete[] m_tot;
    m_tot = nullptr;
    delete[] m_tot_arty;
    m_tot_arty = nullptr;
    delete[] m_field;
    m_field = nullptr;
    delete[] m_field_arty;
    m_field_arty = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
}

bool LuckyArmyFieldSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0u || save_n == 0u) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    return true;
}

bool LuckyArmyFieldSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (!paths.ok() || paths.save_turn_n() != m_save_n) {
        return false;
    }
    if (!TechAgeMng::ready()) {
        return false;
    }
    if (!UnitUtilityHelper::setup(st)) {
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
    u16 lucky_cap = 0u;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            lucky_cap = static_cast<u16>(lucky_cap + 1u);
        }
    }
    if (lucky_cap == 0u) {
        free_seats(seats, seat_n);
        return false;
    }
    m_lucky = new u16[lucky_cap];
    m_lucky_n = 0u;
    for (u16 p = 0; p < m_player_n; ++p) {
        if (seats[p].m_lucky != 0u) {
            m_lucky[m_lucky_n] = p;
            m_lucky_n = static_cast<u16>(m_lucky_n + 1u);
        }
    }
    free_seats(seats, seat_n);
    const u32 cells = static_cast<u32>(m_lucky_n) * static_cast<u32>(m_save_n);
    m_turn = new u32[m_save_n];
    m_tot = new u32[cells];
    m_tot_arty = new u32[cells];
    m_field = new u32[cells];
    m_field_arty = new u32[cells];
    for (u16 si = 0; si < m_save_n; ++si) {
        m_turn[si] = 0u;
        for (u16 li = 0; li < m_lucky_n; ++li) {
            const u32 ix = static_cast<u32>(li) * static_cast<u32>(m_save_n) + si;
            m_tot[ix] = 0u;
            m_tot_arty[ix] = 0u;
            m_field[ix] = 0u;
            m_field_arty[ix] = 0u;
        }
    }
    const u16 typ_n = st.unit().get_item_count();
    if (typ_n == 0u) {
        return false;
    }
    for (u16 si = 0; si < m_save_n; ++si) {
        const u32 turn = paths.save_turn_at(si);
        m_turn[si] = turn;
        char units_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))) {
            return false;
        }
        UnitAddVector units;
        if (!GameIo::load_units(units_p, units)) {
            return false;
        }
        const u16 head = units.get_head_unit_add_idx();
        for (u16 k = 0; k < head; ++k) {
            const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
            if (u == nullptr || u->m_health == 0u) {
                continue;
            }
            const u16 li = lucky_i(m_lucky, m_lucky_n, u->m_player_idx);
            if (li == U16_KEY_NULL) {
                continue;
            }
            if (u->m_unit_typ_idx >= typ_n) {
                continue;
            }
            const u16 ut = st.unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
            if (!UnitUtilityHelper::is_type_for_land_army(ut)) {
                continue;
            }
            const u32 ix = static_cast<u32>(li) * static_cast<u32>(m_save_n) + si;
            m_tot[ix] = m_tot[ix] + 1u;
            const bool br = can_br(st, u->m_unit_typ_idx);
            if (br) {
                m_tot_arty[ix] = m_tot_arty[ix] + 1u;
            }
            if (u->m_in_army == 0u && u->m_in_campaign == 0u) {
                continue;
            }
            m_field[ix] = m_field[ix] + 1u;
            if (br) {
                m_field_arty[ix] = m_field_arty[ix] + 1u;
            }
        }
    }
    return true;
}

u16 LuckyArmyFieldSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 LuckyArmyFieldSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 LuckyArmyFieldSaveTimelineMng::lucky_n () const {
    return m_lucky_n;
}

u16 LuckyArmyFieldSaveTimelineMng::seat (u16 li) const {
    if (m_lucky == nullptr || li >= m_lucky_n) {
        return U16_KEY_NULL;
    }
    return m_lucky[li];
}

u32 LuckyArmyFieldSaveTimelineMng::turn_at (u16 si) const {
    if (m_turn == nullptr || si >= m_save_n) {
        return 0u;
    }
    return m_turn[si];
}

u32 LuckyArmyFieldSaveTimelineMng::tot_n (u16 li, u16 si) const {
    if (m_tot == nullptr || li >= m_lucky_n || si >= m_save_n) {
        return 0u;
    }
    return m_tot[static_cast<u32>(li) * static_cast<u32>(m_save_n) + si];
}

u32 LuckyArmyFieldSaveTimelineMng::tot_arty_n (u16 li, u16 si) const {
    if (m_tot_arty == nullptr || li >= m_lucky_n || si >= m_save_n) {
        return 0u;
    }
    return m_tot_arty[static_cast<u32>(li) * static_cast<u32>(m_save_n) + si];
}

u32 LuckyArmyFieldSaveTimelineMng::field_n (u16 li, u16 si) const {
    if (m_field == nullptr || li >= m_lucky_n || si >= m_save_n) {
        return 0u;
    }
    return m_field[static_cast<u32>(li) * static_cast<u32>(m_save_n) + si];
}

u32 LuckyArmyFieldSaveTimelineMng::field_arty_n (u16 li, u16 si) const {
    if (m_field_arty == nullptr || li >= m_lucky_n || si >= m_save_n) {
        return 0u;
    }
    return m_field_arty[static_cast<u32>(li) * static_cast<u32>(m_save_n) + si]; 
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
