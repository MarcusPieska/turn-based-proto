//================================================================================================================================
//=> - Includes (mk01: capital origin, 90-deg bins, farthest city per bin) -
//================================================================================================================================

#include <cmath>

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - Locals (mk01) -
//================================================================================================================================

static const u16 k_quad_n = 4u;

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

static u16 bth_quad (i32 dx, i32 dy) {
    const double a = std::atan2(static_cast<double>(dy), static_cast<double>(dx));
    double t = a;
    if (t < 0.0) {
        t = t + 6.283185307179586;
    }
    u16 q = static_cast<u16>(t / 1.5707963267948966);
    if (q >= k_quad_n) {
        q = static_cast<u16>(k_quad_n - 1u);
    }
    return q;
}

//================================================================================================================================
//=> - BorderTurnHandler::handle (mk01) -
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
    u16 used[k_front_cap];
    u16 used_n = 0u;
    gather_used(fronts, n, used, &used_n);
    const u16 cx = cap->get_x();
    const u16 cy = cap->get_y();
    bool occ[k_quad_n];
    for (u16 q = 0; q < k_quad_n; ++q) {
        occ[q] = false;
    }
    for (u16 u = 0; u < used_n; ++u) {
        const City* uc = m_cities->get_city(used[u]);
        if (uc == nullptr) {
            continue;
        }
        const i32 dx = static_cast<i32>(uc->get_x()) - static_cast<i32>(cx);
        const i32 dy = static_cast<i32>(uc->get_y()) - static_cast<i32>(cy);
        if (dx == 0 && dy == 0) {
            continue;
        }
        occ[bth_quad(dx, dy)] = true;
    }
    u16 best_i[k_quad_n];
    u32 best_d2[k_quad_n];
    for (u16 q = 0; q < k_quad_n; ++q) {
        best_i[q] = U16_KEY_NULL;
        best_d2[q] = 0u;
    }
    const u16 cn = m_cities->get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        if (i == m_cap) {
            continue;
        }
        const City* c = m_cities->get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        bool skip = false;
        for (u16 u = 0; u < used_n; ++u) {
            if (used[u] == i) {
                skip = true;
                break;
            }
        }
        if (skip) {
            continue;
        }
        const i32 dx = static_cast<i32>(c->get_x()) - static_cast<i32>(cx);
        const i32 dy = static_cast<i32>(c->get_y()) - static_cast<i32>(cy);
        if (dx == 0 && dy == 0) {
            continue;
        }
        const u16 q = bth_quad(dx, dy);
        const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
        if (best_i[q] == U16_KEY_NULL || d2 > best_d2[q]) {
            best_i[q] = i;
            best_d2[q] = d2;
        }
    }
    BorderFront picks[k_front_cap];
    u16 pick_n = 0u;
    for (u16 q = 0; q < k_quad_n && pick_n < need; ++q) {
        if (occ[q] || best_i[q] == U16_KEY_NULL) {
            continue;
        }
        picks[pick_n].m_city = best_i[q];
        picks[pick_n].m_pl = U16_KEY_NULL;
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
