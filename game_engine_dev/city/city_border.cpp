//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_border.h"

#include "assert_log.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - Static data -
//================================================================================================================================

const u16 CityBorder::m_lims[] = {
    10, 25, 50, 100, 150, 250, 500, 750, 1000, 1500,
    2000, 3000, 5000, 7500, 10000, 15000, 20000, 30000, 40000, 50000
};

const u16 CityBorder::m_lims_n = static_cast<u16>(sizeof(CityBorder::m_lims) / sizeof(CityBorder::m_lims[0]));
GameArraySimple* CityBorder::m_map = nullptr;

//================================================================================================================================
//=> - CityBorder -
//================================================================================================================================

void CityBorder::bind_map (GameArraySimple* map) {
    m_map = map;
}

CircArea CityBorder::get (u16 radius) {
    return CircularTileAreas::get(radius);
}

bool CityBorder::will_expand (u16 old_culture, u16 new_culture) {
    return radius_for(old_culture) != radius_for(new_culture);
}

u16 CityBorder::radius_for (u16 culture) {
    u16 r = 0;
    while (r < m_lims_n && culture >= m_lims[r]) {
        ++r;
    }
    return r;
}

void CityBorder::claim_expand (u16 cx, u16 cy, u16 old_culture, u16 new_culture, u8 owner) {
    GAME_EXPECT(m_map != nullptr, "CityBorder map");
    const u16 r0 = radius_for(old_culture);
    const u16 r1 = radius_for(new_culture);
    if (r1 <= r0) {
        return;
    }
    const CircArea area = get(r1);
    for (u16 i = 0; i < area.m_lim; ++i) {
        const i32 x = static_cast<i32>(cx) + static_cast<i32>(area.m_brd[i][0]);
        const i32 y = static_cast<i32>(cy) + static_cast<i32>(area.m_brd[i][1]);
        if (x < 0 || y < 0) {
            continue;
        }
        const u16 ux = static_cast<u16>(x);
        const u16 uy = static_cast<u16>(y);
        if (ux >= m_map->width() || uy >= m_map->height()) {
            continue;
        }
        const u8 cur = m_map->get_civ_owner(ux, uy);
        if (cur != U8_KEY_NULL && cur != owner) {
            continue;
        }
        m_map->set_civ_owner(ux, uy, owner);
    }
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
