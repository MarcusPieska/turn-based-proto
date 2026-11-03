//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_select_greedy.h"

#include "city.h"
#include "game_state.h"
#include "sector_support.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static u32 csg_man (u16 ax, u16 ay, u16 bx, u16 by) {
    const u32 dx = (ax > bx) ? static_cast<u32>(ax - bx) : static_cast<u32>(bx - ax);
    const u32 dy = (ay > by) ? static_cast<u32>(ay - by) : static_cast<u32>(by - ay);
    return dx + dy;
}

//================================================================================================================================
//=> - CitySelect_Greedy -
//================================================================================================================================

u16 CitySelect_Greedy::fill (
    GameState& st,
    u16 sector,
    u16 enemy,
    u16 ax,
    u16 ay,
    u16* out,
    u16 cap) {
    if (out == nullptr || cap == 0u) {
        return 0u;
    }
    const u16 lim = (cap < k_cap) ? cap : k_cap;
    u16 cand[k_cap];
    const u16 n = SectorSupport::enemy_cities(sector, enemy, cand, lim);
    if (n == 0u) {
        return 0u;
    }
    bool take[k_cap];
    for (u16 i = 0; i < n; ++i) {
        take[i] = false;
    }
    u16 cx = ax;
    u16 cy = ay;
    for (u16 o = 0; o < n; ++o) {
        u16 bi = U16_KEY_NULL;
        u32 bd = 0xFFFFFFFFu;
        for (u16 i = 0; i < n; ++i) {
            if (take[i]) {
                continue;
            }
            const City* c = st.m_cities.get_city(cand[i]);
            if (c == nullptr) {
                continue;
            }
            const u32 d = csg_man(cx, cy, c->get_x(), c->get_y());
            if (d < bd || (d == bd && (bi == U16_KEY_NULL || cand[i] < cand[bi]))) {
                bd = d;
                bi = i;
            }
        }
        if (bi == U16_KEY_NULL) {
            return o;
        }
        take[bi] = true;
        out[o] = cand[bi];
        const City* c = st.m_cities.get_city(cand[bi]);
        if (c != nullptr) {
            cx = c->get_x();
            cy = c->get_y();
        }
    }
    return n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
