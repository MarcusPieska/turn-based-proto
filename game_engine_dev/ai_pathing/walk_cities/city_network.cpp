//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_network.h"

#include "city_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static int quad_of_xy (u16 sx, u16 sy, u16 dx, u16 dy) {
    const i32 qx = static_cast<i32>(dx) - static_cast<i32>(sx);
    const i32 qy = static_cast<i32>(dy) - static_cast<i32>(sy);
    if (qx == 0 && qy == 0) {
        return -1;
    }
    if (qx >= 0 && qy < 0) {
        return 0;
    }
    if (qx < 0 && qy <= 0) {
        return 1;
    }
    if (qx <= 0 && qy > 0) {
        return 3;
    }
    return 2;
}

static bool tile_walk (u8 terr) {
    if (overlay_is_water_terr(terr)) {
        return false;
    }
    if (terr == TERR_MOUNTAINS[0] || terr == TERR_NONE[0]) {
        return false;
    }
    return true;
}

static bool pref_cand (u8 n_best, u16 j_best, u8 n_j, u16 j) {
    if (n_j < n_best) {
        return true;
    }
    if (n_j > n_best) {
        return false;
    }
    return j > j_best;
}

//================================================================================================================================
//=> - CityNetwork -
//================================================================================================================================

CityNetwork::CityNetwork () :
    m_ok(false),
    m_city_n(0),
    m_cap(0),
    m_flood_n(0),
    m_cities(nullptr),
    m_map(nullptr) {
}

CityNetwork::~CityNetwork () {
    clear();
}

void CityNetwork::clear () {
    m_cities = nullptr;
    m_map = nullptr;
    m_city_n = 0;
    m_cap = 0;
    m_flood_n = 0;
    m_ok = false;
}

u16 CityNetwork::pos_x (u16 i) const {
    return m_cities->get_city(i)->get_x();
}

u16 CityNetwork::pos_y (u16 i) const {
    return m_cities->get_city(i)->get_y();
}

bool CityNetwork::tile_pass (u16 x, u16 y) const {
    if (m_map == nullptr || x >= m_map->width() || y >= m_map->height()) {
        return false;
    }
    return tile_walk(m_map->get_terrain(x, y));
}

bool CityNetwork::in_rng (u16 a, u16 b) const {
    const i32 dx = static_cast<i32>(pos_x(a)) - static_cast<i32>(pos_x(b));
    const i32 dy = static_cast<i32>(pos_y(a)) - static_cast<i32>(pos_y(b));
    const i32 ax = (dx < 0) ? -dx : dx;
    const i32 ay = (dy < 0) ? -dy : dy;
    const i32 r = static_cast<i32>(CN_LINK_RANGE);
    return ax <= r && ay <= r;
}

u8 CityNetwork::link_n (u16 i) const {
    const City* c = m_cities->get_city(i);
    u8 n = 0;
    for (u8 d = 0; d < 4u; ++d) {
        if (c->get_conn_city(d) != U16_KEY_NULL) {
            ++n;
        }
    }
    return n;
}

void CityNetwork::push_disc (CnDisc* disc, u32* disc_n, u16 id, int q) {
    if (disc == nullptr || disc_n == nullptr || q < 0 || id >= m_city_n) {
        return;
    }
    if (*disc_n >= CN_DISC_MAX) {
        return;
    }
    const u8 q8 = static_cast<u8>(q);
    for (u32 i = 0; i < *disc_n; ++i) {
        if (disc[i].m_id == id && disc[i].m_q == q8) {
            return;
        }
    }
    disc[*disc_n].m_id = id;
    disc[*disc_n].m_q = q8;
    disc[*disc_n].m_was = m_cities->get_city(id)->get_conn_city(q8);
    ++(*disc_n);
}

void CityNetwork::flood_link (u16 src) {
    if (m_map == nullptr || m_cities == nullptr || src >= m_city_n) {
        return;
    }
    ++m_flood_n;
    const u16 sx = pos_x(src);
    const u16 sy = pos_y(src);
    const i32 half = static_cast<i32>(CN_LINK_RANGE);
    const i32 ox = static_cast<i32>(sx) - half;
    const i32 oy = static_cast<i32>(sy) - half;
    for (u32 i = 0; i < CN_WIN_N; ++i) {
        m_wd[i] = U16_KEY_NULL;
    }
    u16 best_id[4] = {U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL};
    u16 best_d[4] = {U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL, U16_KEY_NULL};
    const u32 sidx = static_cast<u32>(half) * CN_WIN + static_cast<u32>(half);
    m_wd[sidx] = 0;
    u32 qn = 0;
    m_qx[qn] = static_cast<u8>(half);
    m_qy[qn] = static_cast<u8>(half);
    ++qn;
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    const u16 mw = m_map->width();
    const u16 mh = m_map->height();
    for (u32 qh = 0; qh < qn; ++qh) {
        const u8 lx = m_qx[qh];
        const u8 ly = m_qy[qh];
        const u16 d0 = m_wd[static_cast<u32>(ly) * CN_WIN + static_cast<u32>(lx)];
        const i32 wx = ox + static_cast<i32>(lx);
        const i32 wy = oy + static_cast<i32>(ly);
        if (wx >= 0 && wy >= 0 && wx < static_cast<i32>(mw) && wy < static_cast<i32>(mh)) {
            const u16 ux = static_cast<u16>(wx);
            const u16 uy = static_cast<u16>(wy);
            if (m_map->get_add_typ(ux, uy) == BUILD_ADD_CITY) {
                const u16 j = m_map->get_add_idx(ux, uy);
                if (j != src && j < m_city_n && d0 > 0 && d0 <= CN_LINK_RANGE) {
                    const int q = quad_of_xy(sx, sy, ux, uy);
                    if (q >= 0) {
                        if (best_id[q] == U16_KEY_NULL || d0 < best_d[q]) {
                            best_id[q] = j;
                            best_d[q] = d0;
                        }
                    }
                }
            }
        }
        if (best_id[0] != U16_KEY_NULL && best_id[1] != U16_KEY_NULL &&
            best_id[2] != U16_KEY_NULL && best_id[3] != U16_KEY_NULL) {
            break;
        }
        if (d0 >= CN_LINK_RANGE) {
            continue;
        }
        for (i32 dir = 0; dir < 4; ++dir) {
            const i32 nlx = static_cast<i32>(lx) + dx4[dir];
            const i32 nly = static_cast<i32>(ly) + dy4[dir];
            if (nlx < 0 || nly < 0 || nlx >= static_cast<i32>(CN_WIN) || nly >= static_cast<i32>(CN_WIN)) {
                continue;
            }
            const u32 nidx = static_cast<u32>(nly) * CN_WIN + static_cast<u32>(nlx);
            if (m_wd[nidx] != U16_KEY_NULL) {
                continue;
            }
            const i32 nx = ox + nlx;
            const i32 ny = oy + nly;
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(mw) || ny >= static_cast<i32>(mh)) {
                continue;
            }
            if (!tile_pass(static_cast<u16>(nx), static_cast<u16>(ny))) {
                continue;
            }
            m_wd[nidx] = static_cast<u16>(d0 + 1u);
            m_qx[qn] = static_cast<u8>(nlx);
            m_qy[qn] = static_cast<u8>(nly);
            ++qn;
        }
    }
    CnDisc disc[CN_DISC_MAX];
    for (int q = 0; q < 4; ++q) {
        if (best_id[q] == U16_KEY_NULL) {
            continue;
        }
        u32 disc_n = 0;
        bind_pair(src, best_id[q], disc, &disc_n);
        drain_repairs(disc, &disc_n);
    }
}

u16 CityNetwork::pick_rep (u16 src, int q, const CnDisc* disc, u32 disc_n) const {
    if (q < 0 || src >= m_city_n || disc == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 sx = pos_x(src);
    const u16 sy = pos_y(src);
    u16 best_j = U16_KEY_NULL;
    u8 best_n = 255;
    for (u32 k = 0; k < disc_n; ++k) {
        const u16 j = disc[k].m_id;
        if (j == src || j >= m_city_n) {
            continue;
        }
        if (quad_of_xy(sx, sy, pos_x(j), pos_y(j)) != q) {
            continue;
        }
        if (!in_rng(src, j)) {
            continue;
        }
        const u8 n_j = link_n(j);
        if (best_j == U16_KEY_NULL || pref_cand(best_n, best_j, n_j, j)) {
            best_j = j;
            best_n = n_j;
        }
    }
    return best_j;
}

void CityNetwork::bind_pair (u16 a, u16 b, CnDisc* disc, u32* disc_n) {
    const int qa = quad_of_xy(pos_x(a), pos_y(a), pos_x(b), pos_y(b));
    const int qb = quad_of_xy(pos_x(b), pos_y(b), pos_x(a), pos_y(a));
    if (qa < 0 || qb < 0) {
        return;
    }
    const u8 da = static_cast<u8>(qa);
    const u8 db = static_cast<u8>(qb);
    City* ca = m_cities->get_city(a);
    City* cb = m_cities->get_city(b);
    if (ca->is_conn_city_locked(da) || cb->is_conn_city_locked(db)) {
        return;
    }
    const u16 old_a = ca->get_conn_city(da);
    const u16 old_b = cb->get_conn_city(db);
    if (old_a == b && old_b == a) {
        return;
    }
    if (old_a != U16_KEY_NULL && old_a != b) {
        const int oq = quad_of_xy(pos_x(old_a), pos_y(old_a), pos_x(a), pos_y(a));
        if (oq >= 0) {
            City* oc = m_cities->get_city(old_a);
            const u8 od = static_cast<u8>(oq);
            if (!oc->is_conn_city_locked(od) && oc->get_conn_city(od) == a) {
                push_disc(disc, disc_n, old_a, oq);
                oc->set_conn_city(U16_KEY_NULL, od);
            }
        }
    }
    if (old_b != U16_KEY_NULL && old_b != a) {
        const int oq = quad_of_xy(pos_x(old_b), pos_y(old_b), pos_x(b), pos_y(b));
        if (oq >= 0) {
            City* oc = m_cities->get_city(old_b);
            const u8 od = static_cast<u8>(oq);
            if (!oc->is_conn_city_locked(od) && oc->get_conn_city(od) == b) {
                push_disc(disc, disc_n, old_b, oq);
                oc->set_conn_city(U16_KEY_NULL, od);
            }
        }
    }
    ca->set_conn_city(b, da);
    cb->set_conn_city(a, db);
}

void CityNetwork::drain_repairs (CnDisc* disc, u32* disc_n) {
    if (disc == nullptr || disc_n == nullptr) {
        return;
    }
    u32 head = 0;
    while (head < *disc_n) {
        const u16 i = disc[head].m_id;
        const int q = static_cast<int>(disc[head].m_q);
        ++head;
        if (i >= m_city_n || q < 0) {
            continue;
        }
        City* c = m_cities->get_city(i);
        const u8 d = static_cast<u8>(q);
        if (c->is_conn_city_locked(d) || c->get_conn_city(d) != U16_KEY_NULL) {
            continue;
        }
        const u16 best = pick_rep(i, q, disc, *disc_n);
        if (best == U16_KEY_NULL) {
            continue;
        }
        bind_pair(i, best, disc, disc_n);
    }
}

bool CityNetwork::begin (CityArray& cities, GameArraySimple& map) {
    clear();
    if (map.width() == 0 || map.height() == 0) {
        return false;
    }
    const u32 cap32 = static_cast<u32>(CityArray::MAX_PAGES) * static_cast<u32>(CityArray::CITIES_PER_PAGE);
    if (cap32 <= 1u) {
        return false;
    }
    const u16 cap = static_cast<u16>((cap32 > 65535u) ? 65535u : (cap32 - 1u));
    if (cap == 0) {
        return false;
    }
    m_cap = cap;
    m_city_n = 0;
    m_cities = &cities;
    m_map = &map;
    m_ok = true;
    return true;
}

bool CityNetwork::add (u16 city_idx) {
    if (!m_ok || m_cities == nullptr || m_map == nullptr) {
        return false;
    }
    if (city_idx != m_city_n || city_idx >= m_cap) {
        return false;
    }
    if (m_cities->get_city(city_idx) == nullptr) {
        return false;
    }
    City* c = m_cities->get_city(city_idx);
    for (u8 d = 0; d < 4u; ++d) {
        c->set_conn_city(U16_KEY_NULL, d);
    }
    ++m_city_n;
    flood_link(city_idx);
    return true;
}

bool CityNetwork::is_valid () const {
    return m_ok;
}

u16 CityNetwork::city_n () const {
    return m_city_n;
}

u32 CityNetwork::flood_n () const {
    return m_flood_n;
}

const CityArray* CityNetwork::cities () const {
    return m_cities;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
