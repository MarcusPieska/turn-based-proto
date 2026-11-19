//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_army_prod_save_timeline_mng.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "eval_paths.h"
#include "game_io.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "tech_age_mng.h"
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

static int cmp_u16 (const void* a, const void* b) {
    const u16 x = *static_cast<const u16*>(a);
    const u16 y = *static_cast<const u16*>(b);
    if (x < y) {
        return -1;
    }
    if (x > y) {
        return 1;
    }
    return 0;
}

static bool has_key (const u16* keys, u16 n, u16 key) {
    if (keys == nullptr || n == 0u) {
        return false;
    }
    return std::bsearch(&key, keys, n, sizeof(u16), cmp_u16) != nullptr;
}

static bool collect_army_keys (
    const UnitAddVector& units,
    const RuntimeStatics& st,
    u16 seat,
    u16* out,
    u16 cap,
    u16* out_n) {
    if (out == nullptr || out_n == nullptr || cap == 0u) {
        return false;
    }
    *out_n = 0u;
    const u16 typ_n = st.unit().get_item_count();
    if (typ_n == 0u) {
        return false;
    }
    const u16 head = units.get_head_unit_add_idx();
    for (u16 k = 0; k < head; ++k) {
        const UnitAddStruct* u = units.get_unit_add(UnitAddKey::from_raw(k));
        if (u == nullptr || u->m_x == U16_KEY_NULL || u->m_player_idx != seat) {
            continue;
        }
        if (u->m_unit_typ_idx >= typ_n) {
            continue;
        }
        const u16 ut = st.unit().get_item(UnitStaticDataKey::from_raw(u->m_unit_typ_idx)).type;
        if (!UnitUtilityHelper::is_type_for_land_army(ut)) {
            continue;
        }
        if (*out_n >= cap) {
            break;
        }
        out[*out_n] = k;
        *out_n = static_cast<u16>(*out_n + 1u);
    }
    if (*out_n > 1u) {
        std::qsort(out, *out_n, sizeof(u16), cmp_u16);
    }
    return true;
}

//================================================================================================================================
//=> - LuckyArmyProdSaveTimelineMng -
//================================================================================================================================

LuckyArmyProdSaveTimelineMng::LuckyArmyProdSaveTimelineMng ()
    : m_lucky(nullptr),
      m_turn_end(nullptr),
      m_new(nullptr),
      m_lucky_n(0),
      m_player_n(0),
      m_save_n(0),
      m_iv_n(0) {
}

LuckyArmyProdSaveTimelineMng::~LuckyArmyProdSaveTimelineMng () {
    clr();
}

void LuckyArmyProdSaveTimelineMng::clr () {
    delete[] m_lucky;
    m_lucky = nullptr;
    delete[] m_turn_end;
    m_turn_end = nullptr;
    delete[] m_new;
    m_new = nullptr;
    m_lucky_n = 0;
    m_player_n = 0;
    m_save_n = 0;
    m_iv_n = 0;
}

bool LuckyArmyProdSaveTimelineMng::setup (u16 player_n, u16 save_n) {
    clr();
    if (player_n == 0u || save_n < 2u) {
        return false;
    }
    m_player_n = player_n;
    m_save_n = save_n;
    m_iv_n = static_cast<u16>(save_n - 1u);
    return true;
}

bool LuckyArmyProdSaveTimelineMng::fill (const EvalPaths& paths, const RuntimeStatics& st) {
    if (!paths.ok() || paths.save_turn_n() != m_save_n || m_iv_n == 0u) {
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
    m_turn_end = new u32[m_iv_n];
    m_new = new u32[static_cast<u32>(m_lucky_n) * static_cast<u32>(m_iv_n)];
    for (u16 ii = 0; ii < m_iv_n; ++ii) {
        m_turn_end[ii] = 0u;
        for (u16 li = 0; li < m_lucky_n; ++li) {
            m_new[static_cast<u32>(li) * static_cast<u32>(m_iv_n) + ii] = 0u;
        }
    }
    static const u16 k_cap = 8192u;
    u16** prev = new u16*[m_lucky_n];
    u16* prev_n = new u16[m_lucky_n];
    u16* cur = new u16[k_cap];
    for (u16 li = 0; li < m_lucky_n; ++li) {
        prev[li] = new u16[k_cap];
        prev_n[li] = 0u;
    }
    bool ok = true;
    for (u16 si = 0; si < m_save_n && ok; ++si) {
        const u32 turn = paths.save_turn_at(si);
        char units_p[512];
        if (!paths.units_path(turn, units_p, sizeof(units_p))) {
            ok = false;
            break;
        }
        UnitAddVector units;
        if (!GameIo::load_units(units_p, units)) {
            ok = false;
            break;
        }
        for (u16 li = 0; li < m_lucky_n; ++li) {
            u16 cn = 0u;
            if (!collect_army_keys(units, st, m_lucky[li], cur, k_cap, &cn)) {
                ok = false;
                break;
            }
            if (si == 0u) {
                std::memcpy(prev[li], cur, static_cast<size_t>(cn) * sizeof(u16));
                prev_n[li] = cn;
                continue;
            }
            const u16 ii = static_cast<u16>(si - 1u);
            m_turn_end[ii] = turn;
            u32 neu = 0u;
            for (u16 i = 0; i < cn; ++i) {
                if (!has_key(prev[li], prev_n[li], cur[i])) {
                    neu = neu + 1u;
                }
            }
            m_new[static_cast<u32>(li) * static_cast<u32>(m_iv_n) + ii] = neu;
            std::memcpy(prev[li], cur, static_cast<size_t>(cn) * sizeof(u16));
            prev_n[li] = cn;
        }
    }
    for (u16 li = 0; li < m_lucky_n; ++li) {
        delete[] prev[li];
    }
    delete[] prev;
    delete[] prev_n;
    delete[] cur;
    return ok;
}

u16 LuckyArmyProdSaveTimelineMng::player_n () const {
    return m_player_n;
}

u16 LuckyArmyProdSaveTimelineMng::save_n () const {
    return m_save_n;
}

u16 LuckyArmyProdSaveTimelineMng::lucky_n () const {
    return m_lucky_n;
}

u16 LuckyArmyProdSaveTimelineMng::interval_n () const {
    return m_iv_n;
}

u16 LuckyArmyProdSaveTimelineMng::seat (u16 li) const {
    if (m_lucky == nullptr || li >= m_lucky_n) {
        return U16_KEY_NULL;
    }
    return m_lucky[li];
}

u32 LuckyArmyProdSaveTimelineMng::turn_end (u16 ii) const {
    if (m_turn_end == nullptr || ii >= m_iv_n) {
        return 0u;
    }
    return m_turn_end[ii];
}

u32 LuckyArmyProdSaveTimelineMng::new_n (u16 li, u16 ii) const {
    if (m_new == nullptr || li >= m_lucky_n || ii >= m_iv_n) {
        return 0u;
    }
    return m_new[static_cast<u32>(li) * static_cast<u32>(m_iv_n) + ii];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
