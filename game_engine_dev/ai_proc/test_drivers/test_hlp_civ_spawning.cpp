//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "test_hlp_civ_spawning.h"

#include "city.h"
#include "civ_relations.h"
#include "civ_static_key.h"
#include "game_state.h"
#include "map_bit_overlay.h"
#include "runtime_statics.h"
#include "test_hlp_map_load.h"

//================================================================================================================================
//=> - TestHlpCivSpawning -
//================================================================================================================================

bool TestHlpCivSpawning::init_seats (GameState& s, u16 seat_n, u16 view, u16 enemy) {
    RuntimeStatics* st = TestHlpMapLoad::statics();
    if (st == nullptr || seat_n == 0 || view >= seat_n || enemy >= seat_n || enemy == view) {
        return false;
    }
    const u16 civ_n = st->civ().get_item_count();
    if (civ_n < seat_n) {
        return false;
    }
    const u16 w = s.m_map.width();
    const u16 h = s.m_map.height();
    PlayerState* seats = new PlayerState[seat_n];
    if (seats == nullptr) {
        return false;
    }
    for (u16 i = 0; i < seat_n; ++i) {
        seats[i].m_ai_controlled = 1;
        seats[i].m_is_active = 1;
        seats[i].m_civ_index = i;
        seats[i].m_explored_overlay = new MapBitOverlay(w, h);
        if (seats[i].m_explored_overlay == nullptr) {
            for (u16 j = 0; j <= i; ++j) {
                delete seats[j].m_explored_overlay;
                seats[j].m_explored_overlay = nullptr;
            }
            delete[] seats;
            return false;
        }
    }
    s.m_player_states = seats;
    s.m_player_n = seat_n;
    s.m_players_remaining = seat_n;
    s.m_civ_relations.reset(civ_n);
    const CivStaticDataKey skv = CivStaticDataKey::from_raw(view);
    for (u16 i = 0; i < seat_n; ++i) {
        if (i == view) {
            continue;
        }
        const CivRel rel = (i == enemy) ? CivRel::CIV_REL_PEACE : CivRel::CIV_REL_WAR;
        s.m_civ_relations.set(skv, CivStaticDataKey::from_raw(i), rel);
    }
    City::bind_player_states(s.m_player_states, s.m_player_n);
    City::bind_units(&s.m_units);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
