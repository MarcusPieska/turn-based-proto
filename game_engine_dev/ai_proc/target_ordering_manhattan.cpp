//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "target_ordering_manhattan.h"

#include "city.h"
#include "city_array.h"
#include "game_state.h"

//================================================================================================================================
//=> - TargetOrderingManhattan -
//================================================================================================================================

bool TargetOrderingManhattan::pick (
    const GameState& s,
    u16 enemy,
    u16 from_x,
    u16 from_y,
    u16* ox,
    u16* oy) {
    if (ox == nullptr || oy == nullptr) {
        return false;
    }
    if (enemy >= s.m_player_n) {
        return false;
    }
    const u16 cn = s.m_cities.get_city_count();
    if (cn == 0u) {
        return false;
    }
    u32 best_d = 0xFFFFFFFFu;
    u16 bx = 0;
    u16 by = 0;
    bool found = false;
    for (u16 i = 0; i < cn; ++i) {
        const City* c = s.m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != enemy) {
            continue;
        }
        const u16 ex = c->get_x();
        const u16 ey = c->get_y();
        const u32 adx = from_x > ex ? static_cast<u32>(from_x - ex) : static_cast<u32>(ex - from_x);
        const u32 ady = from_y > ey ? static_cast<u32>(from_y - ey) : static_cast<u32>(ey - from_y);
        const u32 d = adx + ady;
        if (!found || d < best_d) {
            best_d = d;
            bx = ex;
            by = ey;
            found = true;
        }
    }
    if (!found) {
        return false;
    }
    *ox = bx;
    *oy = by;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
