//================================================================================================================================
//=> - Includes (mk06: city-array distance; fill up to need slots in fronts[0..n)) -
//================================================================================================================================

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - Locals (mk06) -
//================================================================================================================================

static const u16 k_pl_cap = 512u;

static u16 bth_find_cap (const CityArray& cities, u16 seat) {
    const u16 cn = cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c != nullptr && c->get_owner() == seat) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

static u32 bth_man (u16 ax, u16 ay, u16 bx, u16 by) {
    const u32 adx = ax > bx ? static_cast<u32>(ax - bx) : static_cast<u32>(bx - ax);
    const u32 ady = ay > by ? static_cast<u32>(ay - by) : static_cast<u32>(by - ay);
    return adx + ady;
}

static bool bth_used (u16 city, const u16* used, u16 used_n) {
    for (u16 u = 0; u < used_n; ++u) {
        if (used[u] == city) {
            return true;
        }
    }
    return false;
}

static u16 bth_own_near (
    const CityArray& cities,
    u16 seat,
    u16 tx,
    u16 ty,
    const u16* used,
    u16 used_n) {
    u16 best = U16_KEY_NULL;
    u32 best_d = 0xFFFFFFFFu;
    const u16 cn = cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat || bth_used(i, used, used_n)) {
            continue;
        }
        const u32 d = bth_man(tx, ty, c->get_x(), c->get_y());
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

//================================================================================================================================
//=> - BorderTurnHandler::handle (mk06) -
//================================================================================================================================

bool BorderTurnHandler::handle (u16 seat, BorderFront* fronts, u16 n) {
    clr();
    if (!m_ok || m_map == nullptr || m_cities == nullptr || fronts == nullptr) {
        return false;
    }
    if (n == 0u || n > k_front_cap) {
        return false;
    }
    m_cap = bth_find_cap(*m_cities, seat);
    if (m_cap == U16_KEY_NULL) {
        return false;
    }
    const City* cap = m_cities->get_city(m_cap);
    if (cap == nullptr) {
        m_cap = U16_KEY_NULL;
        return false;
    }
    const u16 need = count_need(fronts, n);
    if (need == 0u) {
        sync_from(fronts, n);
        return true;
    }
    const u16 cx = cap->get_x();
    const u16 cy = cap->get_y();
    u16 used[k_front_cap];
    u16 used_n = 0u;
    gather_used(fronts, n, used, &used_n);
    u16 near_i[k_pl_cap];
    u32 near_d[k_pl_cap];
    for (u16 p = 0; p < k_pl_cap; ++p) {
        near_i[p] = U16_KEY_NULL;
        near_d[p] = 0xFFFFFFFFu;
    }
    const u16 cn = m_cities->get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_cities->get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 ow = c->get_owner();
        if (ow == seat || ow == U16_KEY_NULL || ow >= k_pl_cap) {
            continue;
        }
        const u32 d = bth_man(cx, cy, c->get_x(), c->get_y());
        if (d < near_d[ow]) {
            near_d[ow] = d;
            near_i[ow] = i;
        }
    }
    for (u16 i = 0; i < n; ++i) {
        if (!slot_city(fronts[i].m_city)) {
            continue;
        }
        const u16 pl = fronts[i].m_pl;
        if (pl != U16_KEY_NULL && pl < k_pl_cap) {
            near_i[pl] = U16_KEY_NULL;
        }
    }
    u16 hit_x[k_front_cap];
    u16 hit_y[k_front_cap];
    u16 hit_pl[k_front_cap];
    u16 hit_n = 0u;
    for (u16 k = 0; k < need; ++k) {
        u16 pick = U16_KEY_NULL;
        u32 pick_d = 0xFFFFFFFFu;
        for (u16 p = 0; p < k_pl_cap; ++p) {
            if (near_i[p] == U16_KEY_NULL) {
                continue;
            }
            if (near_d[p] < pick_d) {
                pick_d = near_d[p];
                pick = p;
            }
        }
        if (pick == U16_KEY_NULL) {
            break;
        }
        const City* fc = m_cities->get_city(near_i[pick]);
        near_i[pick] = U16_KEY_NULL;
        if (fc == nullptr) {
            continue;
        }
        hit_x[hit_n] = fc->get_x();
        hit_y[hit_n] = fc->get_y();
        hit_pl[hit_n] = pick;
        hit_n = static_cast<u16>(hit_n + 1u);
    }
    BorderFront picks[k_front_cap];
    u16 pick_n = 0u;
    for (u16 h = 0; h < hit_n && pick_n < need; ++h) {
        const u16 oi = bth_own_near(*m_cities, seat, hit_x[h], hit_y[h], used, used_n);
        if (oi == U16_KEY_NULL) {
            continue;
        }
        picks[pick_n].m_city = oi;
        picks[pick_n].m_pl = hit_pl[h];
        used[used_n] = oi;
        used_n = static_cast<u16>(used_n + 1u);
        pick_n = static_cast<u16>(pick_n + 1u);
    }
    fill_need(fronts, n, picks, pick_n);
    fallback_cap(fronts, n, m_cap);
    sync_from(fronts, n);
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
