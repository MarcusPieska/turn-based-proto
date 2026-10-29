//================================================================================================================================
//=> - Includes (mk02: mk01 bins + grid nearby foreign count; drop empty fronts) -
//================================================================================================================================

#include <cmath>
#include <cstring>

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - Locals (mk02) -
//================================================================================================================================

static const u16 k_near_man = 64u;
static const u16 k_cell = 64u;
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

struct BthGrid {
    u16* m_fx;
    u16* m_fy;
    u16* m_head;
    u16* m_next;
    u16 m_fn;
    u16 m_gw;
    u16 m_gh;
};

static void bth_grid_free (BthGrid& g) {
    delete[] g.m_fx;
    delete[] g.m_fy;
    delete[] g.m_head;
    delete[] g.m_next;
    g.m_fx = nullptr;
    g.m_fy = nullptr;
    g.m_head = nullptr;
    g.m_next = nullptr;
    g.m_fn = 0u;
    g.m_gw = 0u;
    g.m_gh = 0u;
}

static bool bth_grid_build (BthGrid& g, const CityArray& cities, u16 seat, u16 mw, u16 mh) {
    g.m_fx = nullptr;
    g.m_fy = nullptr;
    g.m_head = nullptr;
    g.m_next = nullptr;
    g.m_fn = 0u;
    g.m_gw = static_cast<u16>((static_cast<u32>(mw) + k_cell - 1u) / k_cell);
    g.m_gh = static_cast<u16>((static_cast<u32>(mh) + k_cell - 1u) / k_cell);
    if (g.m_gw == 0u || g.m_gh == 0u) {
        return false;
    }
    const u16 cn = cities.get_city_count();
    g.m_fx = new u16[cn];
    g.m_fy = new u16[cn];
    if (g.m_fx == nullptr || g.m_fy == nullptr) {
        bth_grid_free(g);
        return false;
    }
    for (u16 i = 0; i < cn; ++i) {
        const City* c = cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 ow = c->get_owner();
        if (ow == seat || ow == U16_KEY_NULL) {
            continue;
        }
        g.m_fx[g.m_fn] = c->get_x();
        g.m_fy[g.m_fn] = c->get_y();
        g.m_fn = static_cast<u16>(g.m_fn + 1u);
    }
    const u32 cells = static_cast<u32>(g.m_gw) * static_cast<u32>(g.m_gh);
    g.m_head = new u16[cells];
    g.m_next = (g.m_fn > 0u) ? new u16[g.m_fn] : nullptr;
    if (g.m_head == nullptr || (g.m_fn > 0u && g.m_next == nullptr)) {
        bth_grid_free(g);
        return false;
    }
    for (u32 i = 0; i < cells; ++i) {
        g.m_head[i] = U16_KEY_NULL;
    }
    for (u16 i = 0; i < g.m_fn; ++i) {
        const u16 cx = static_cast<u16>(g.m_fx[i] / k_cell);
        const u16 cy = static_cast<u16>(g.m_fy[i] / k_cell);
        const u32 ci = static_cast<u32>(cy) * static_cast<u32>(g.m_gw) + static_cast<u32>(cx);
        g.m_next[i] = g.m_head[ci];
        g.m_head[ci] = i;
    }
    return true;
}

static u16 bth_near_tgt (const BthGrid& g, u16 x, u16 y) {
    if (g.m_head == nullptr || g.m_fn == 0u) {
        return 0u;
    }
    const i32 x0 = static_cast<i32>(x) - static_cast<i32>(k_near_man);
    const i32 y0 = static_cast<i32>(y) - static_cast<i32>(k_near_man);
    const i32 x1 = static_cast<i32>(x) + static_cast<i32>(k_near_man);
    const i32 y1 = static_cast<i32>(y) + static_cast<i32>(k_near_man);
    i32 cx0 = x0 / static_cast<i32>(k_cell);
    i32 cy0 = y0 / static_cast<i32>(k_cell);
    i32 cx1 = x1 / static_cast<i32>(k_cell);
    i32 cy1 = y1 / static_cast<i32>(k_cell);
    if (cx0 < 0) {
        cx0 = 0;
    }
    if (cy0 < 0) {
        cy0 = 0;
    }
    if (cx1 >= static_cast<i32>(g.m_gw)) {
        cx1 = static_cast<i32>(g.m_gw) - 1;
    }
    if (cy1 >= static_cast<i32>(g.m_gh)) {
        cy1 = static_cast<i32>(g.m_gh) - 1;
    }
    u16 n = 0u;
    for (i32 cy = cy0; cy <= cy1; ++cy) {
        for (i32 cx = cx0; cx <= cx1; ++cx) {
            const u32 ci = static_cast<u32>(cy) * static_cast<u32>(g.m_gw) + static_cast<u32>(cx);
            for (u16 i = g.m_head[ci]; i != U16_KEY_NULL; i = g.m_next[i]) {
                const u32 adx = x > g.m_fx[i]
                    ? static_cast<u32>(x - g.m_fx[i]) : static_cast<u32>(g.m_fx[i] - x);
                const u32 ady = y > g.m_fy[i]
                    ? static_cast<u32>(y - g.m_fy[i]) : static_cast<u32>(g.m_fy[i] - y);
                if (adx + ady <= static_cast<u32>(k_near_man)) {
                    n = static_cast<u16>(n + 1u);
                }
            }
        }
    }
    return n;
}

//================================================================================================================================
//=> - BorderTurnHandler::handle (mk02) -
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
    BthGrid grid;
    if (!bth_grid_build(grid, *m_cities, seat, m_map->width(), m_map->height())) {
        return false;
    }
    u16 best_i[k_quad_n];
    u16 best_n[k_quad_n];
    u32 best_d2[k_quad_n];
    for (u16 q = 0; q < k_quad_n; ++q) {
        best_i[q] = U16_KEY_NULL;
        best_n[q] = 0u;
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
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        const i32 dx = static_cast<i32>(x) - static_cast<i32>(cx);
        const i32 dy = static_cast<i32>(y) - static_cast<i32>(cy);
        if (dx == 0 && dy == 0) {
            continue;
        }
        const u16 tn = bth_near_tgt(grid, x, y);
        if (tn == 0u) {
            continue;
        }
        const u16 q = bth_quad(dx, dy);
        const u32 d2 = static_cast<u32>(dx * dx + dy * dy);
        if (best_i[q] == U16_KEY_NULL
            || tn > best_n[q]
            || (tn == best_n[q] && d2 > best_d2[q])) {
            best_i[q] = i;
            best_n[q] = tn;
            best_d2[q] = d2;
        }
    }
    bth_grid_free(grid);
    BorderFront picks[k_front_cap];
    u16 pick_n = 0u;
    for (u16 q = 0; q < k_quad_n && pick_n < need; ++q) {
        if (occ[q] || best_i[q] == U16_KEY_NULL) {
            continue;
        }
        if (m_cities->get_city(best_i[q]) == nullptr) {
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
