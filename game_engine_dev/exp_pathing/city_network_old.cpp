//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_network.h"

#include "city.h"
#include "city_array.h"
#include "game_map_defs.h"
#include "land_mass_index.h"

//================================================================================================================================
//=> - Helpers -
//================================================================================================================================

static const u32 G_LINK_RANGE2 = CN_LINK_RANGE * CN_LINK_RANGE;

static u16 mass_at (const LandMassIndexRslt& mass, u16 x, u16 y) {
    if (mass.m_ov == nullptr || x >= mass.m_w || y >= mass.m_h) {
        return static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    return mass.m_ov[static_cast<u32>(y) * static_cast<u32>(mass.m_w) + static_cast<u32>(x)];
}

static u32 dist2_xy (u16 ax, u16 ay, u16 bx, u16 by) {
    const i32 dx = static_cast<i32>(bx) - static_cast<i32>(ax);
    const i32 dy = static_cast<i32>(by) - static_cast<i32>(ay);
    return static_cast<u32>(dx * dx + dy * dy);
}

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

static u16* link_slot (CityNetLinks& L, int q) {
    if (q == 0) {
        return &L.m_ne;
    }
    if (q == 1) {
        return &L.m_nw;
    }
    if (q == 2) {
        return &L.m_se;
    }
    return &L.m_sw;
}

static void clr_links (CityNetLinks& L) {
    L.m_ne = U16_KEY_NULL;
    L.m_nw = U16_KEY_NULL;
    L.m_se = U16_KEY_NULL;
    L.m_sw = U16_KEY_NULL;
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

static bool line_hard_block (const u8* terr, u16 tw, u16 th, i32 x, i32 y) {
    const u8 t = terr[static_cast<u32>(y) * static_cast<u32>(tw) + static_cast<u32>(x)];
    if (tile_walk(t)) {
        return false;
    }
    static const i32 G_DX[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
    static const i32 G_DY[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
    for (u32 i = 0; i < 8u; ++i) {
        const i32 nx = x + G_DX[i];
        const i32 ny = y + G_DY[i];
        if (nx < 0 || ny < 0 || nx >= static_cast<i32>(tw) || ny >= static_cast<i32>(th)) {
            continue;
        }
        const u8 nt = terr[static_cast<u32>(ny) * static_cast<u32>(tw) + static_cast<u32>(nx)];
        if (tile_walk(nt)) {
            return false;
        }
    }
    return true;
}

static bool quad_aabb (u16 sx, u16 sy, int q, u16 map_w, u16 map_h, i32* x0, i32* x1, i32* y0, i32* y1) {
    const i32 R = static_cast<i32>(CN_LINK_RANGE);
    const i32 mx = static_cast<i32>(map_w) - 1;
    const i32 my = static_cast<i32>(map_h) - 1;
    const i32 x = static_cast<i32>(sx);
    const i32 y = static_cast<i32>(sy);
    if (q == 0) {
        if (y <= 0) {
            return false;
        }
        *x0 = x;
        *x1 = (x + R < mx) ? (x + R) : mx;
        *y0 = (y - R > 0) ? (y - R) : 0;
        *y1 = y - 1;
    } else if (q == 1) {
        if (x <= 0) {
            return false;
        }
        *x0 = (x - R > 0) ? (x - R) : 0;
        *x1 = x - 1;
        *y0 = (y - R > 0) ? (y - R) : 0;
        *y1 = y;
    } else if (q == 2) {
        if (x >= mx) {
            return false;
        }
        *x0 = x + 1;
        *x1 = (x + R < mx) ? (x + R) : mx;
        *y0 = y;
        *y1 = (y + R < my) ? (y + R) : my;
    } else {
        if (y >= my) {
            return false;
        }
        *x0 = (x - R > 0) ? (x - R) : 0;
        *x1 = x;
        *y0 = y + 1;
        *y1 = (y + R < my) ? (y + R) : my;
    }
    return *x0 <= *x1 && *y0 <= *y1;
}

//================================================================================================================================
//=> - CityNetwork -
//================================================================================================================================

CityNetwork::CityNetwork () :
    m_ok(false),
    m_check(CN_CHECK_OFF),
    m_city_n(0),
    m_cap(0),
    m_tw(0),
    m_th(0),
    m_gw(0),
    m_gh(0),
    m_line_n(0),
    m_flood_n(0),
    m_cities(nullptr),
    m_links(nullptr),
    m_masses(nullptr),
    m_cell_head(nullptr),
    m_cell_next(nullptr),
    m_mass(nullptr),
    m_terr(nullptr),
    m_rep_id(nullptr),
    m_rep_q(nullptr),
    m_rep_n(0),
    m_rep_cap(0) {
}

CityNetwork::~CityNetwork () {
    clear();
}

void CityNetwork::clear () {
    delete[] m_links;
    m_links = nullptr;
    delete[] m_masses;
    m_masses = nullptr;
    delete[] m_cell_head;
    m_cell_head = nullptr;
    delete[] m_cell_next;
    m_cell_next = nullptr;
    delete[] m_rep_id;
    m_rep_id = nullptr;
    delete[] m_rep_q;
    m_rep_q = nullptr;
    m_cities = nullptr;
    m_mass = nullptr;
    m_terr = nullptr;
    m_city_n = 0;
    m_cap = 0;
    m_tw = 0;
    m_th = 0;
    m_gw = 0;
    m_gh = 0;
    m_line_n = 0;
    m_flood_n = 0;
    m_rep_n = 0;
    m_rep_cap = 0;
    m_check = CN_CHECK_OFF;
    m_ok = false;
}

u16 CityNetwork::pos_x (u16 i) const {
    return m_cities->get_city(i)->get_x();
}

u16 CityNetwork::pos_y (u16 i) const {
    return m_cities->get_city(i)->get_y();
}

void CityNetwork::init_grid (u16 map_w, u16 map_h) {
    m_gw = static_cast<u16>((static_cast<u32>(map_w) + CN_CELL - 1u) / CN_CELL);
    m_gh = static_cast<u16>((static_cast<u32>(map_h) + CN_CELL - 1u) / CN_CELL);
    const u32 ncell = static_cast<u32>(m_gw) * static_cast<u32>(m_gh);
    m_cell_head = new u16[ncell];
    for (u32 i = 0; i < ncell; ++i) {
        m_cell_head[i] = U16_KEY_NULL;
    }
    m_cell_next = new u16[m_cap];
    for (u16 i = 0; i < m_cap; ++i) {
        m_cell_next[i] = U16_KEY_NULL;
    }
}

void CityNetwork::grid_ins (u16 i) {
    const u16 cx = static_cast<u16>(pos_x(i) / CN_CELL);
    const u16 cy = static_cast<u16>(pos_y(i) / CN_CELL);
    const u32 ci = static_cast<u32>(cy) * static_cast<u32>(m_gw) + static_cast<u32>(cx);
    m_cell_next[i] = m_cell_head[ci];
    m_cell_head[ci] = i;
}

bool CityNetwork::reach (u16 axu, u16 ayu, u16 bxu, u16 byu) {
    ++m_flood_n;
    if (m_terr == nullptr || m_tw == 0 || m_th == 0) {
        return false;
    }
    const i32 ax = static_cast<i32>(axu);
    const i32 ay = static_cast<i32>(ayu);
    const i32 bx = static_cast<i32>(bxu);
    const i32 by = static_cast<i32>(byu);
    const i32 min_x = (ax < bx) ? ax : bx;
    const i32 max_x = (ax > bx) ? ax : bx;
    const i32 min_y = (ay < by) ? ay : by;
    const i32 max_y = (ay > by) ? ay : by;
    const i32 span_x = max_x - min_x;
    const i32 span_y = max_y - min_y;
    if (span_x >= static_cast<i32>(CN_WIN) || span_y >= static_cast<i32>(CN_WIN)) {
        return false;
    }
    const i32 mx = (static_cast<i32>(CN_WIN) - 1 - span_x) / 2;
    const i32 my = (static_cast<i32>(CN_WIN) - 1 - span_y) / 2;
    const i32 ox = min_x - mx;
    const i32 oy = min_y - my;
    for (u32 ly = 0; ly < CN_WIN; ++ly) {
        for (u32 lx = 0; lx < CN_WIN; ++lx) {
            const i32 wx = ox + static_cast<i32>(lx);
            const i32 wy = oy + static_cast<i32>(ly);
            u8 cell = 0;
            if (wx >= 0 && wy >= 0 && wx < static_cast<i32>(m_tw) && wy < static_cast<i32>(m_th)) {
                const u8 t = m_terr[static_cast<u32>(wy) * static_cast<u32>(m_tw) + static_cast<u32>(wx)];
                if (tile_walk(t)) {
                    cell = 1;
                }
            }
            m_win[ly * CN_WIN + lx] = cell;
        }
    }
    const u8 sx = static_cast<u8>(ax - ox);
    const u8 sy = static_cast<u8>(ay - oy);
    const u8 gx = static_cast<u8>(bx - ox);
    const u8 gy = static_cast<u8>(by - oy);
    const u32 sidx = static_cast<u32>(sy) * CN_WIN + static_cast<u32>(sx);
    const u32 gidx = static_cast<u32>(gy) * CN_WIN + static_cast<u32>(gx);
    if (m_win[sidx] == 0 || m_win[gidx] == 0) {
        return false;
    }
    if (sidx == gidx) {
        return true;
    }
    m_win[sidx] = 2;
    u32 qn = 0;
    m_qx[qn] = sx;
    m_qy[qn] = sy;
    ++qn;
    const i32 dx4[4] = {-1, 1, 0, 0};
    const i32 dy4[4] = {0, 0, -1, 1};
    for (u32 qh = 0; qh < qn; ++qh) {
        const u8 cx = m_qx[qh];
        const u8 cy = m_qy[qh];
        if (cx == gx && cy == gy) {
            return true;
        }
        for (i32 d = 0; d < 4; ++d) {
            const i32 nx = static_cast<i32>(cx) + dx4[d];
            const i32 ny = static_cast<i32>(cy) + dy4[d];
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(CN_WIN) || ny >= static_cast<i32>(CN_WIN)) {
                continue;
            }
            const u32 ni = static_cast<u32>(ny) * CN_WIN + static_cast<u32>(nx);
            if (m_win[ni] != 1) {
                continue;
            }
            m_win[ni] = 2;
            m_qx[qn] = static_cast<u8>(nx);
            m_qy[qn] = static_cast<u8>(ny);
            ++qn;
        }
    }
    return m_win[gidx] == 2;
}

bool CityNetwork::line_ok (u16 axu, u16 ayu, u16 bxu, u16 byu) {
    ++m_line_n;
    if (m_terr == nullptr || m_tw == 0 || m_th == 0) {
        return false;
    }
    i32 ax = static_cast<i32>(axu);
    i32 ay = static_cast<i32>(ayu);
    const i32 bx = static_cast<i32>(bxu);
    const i32 by = static_cast<i32>(byu);
    const i32 dx = (bx > ax) ? (bx - ax) : (ax - bx);
    const i32 dy = (by > ay) ? (by - ay) : (ay - by);
    const i32 sx = (ax < bx) ? 1 : -1;
    const i32 sy = (ay < by) ? 1 : -1;
    i32 err = dx - dy;
    for (;;) {
        if (ax < 0 || ay < 0 || ax >= static_cast<i32>(m_tw) || ay >= static_cast<i32>(m_th)) {
            return false;
        }
        if (line_hard_block(m_terr, m_tw, m_th, ax, ay)) {
            return false;
        }
        if (ax == bx && ay == by) {
            return true;
        }
        const i32 e2 = err * 2;
        if (e2 > -dy) {
            err -= dy;
            ax += sx;
        }
        if (e2 < dx) {
            err += dx;
            ay += sy;
        }
    }
}

bool CityNetwork::pair_ok (u16 a, u16 b) {
    const u16 ax = pos_x(a);
    const u16 ay = pos_y(a);
    const u16 bx = pos_x(b);
    const u16 by = pos_y(b);
    const u32 d = dist2_xy(ax, ay, bx, by);
    if (d > G_LINK_RANGE2) {
        return false;
    }
    if (m_check == CN_CHECK_PATHING) {
        return reach(ax, ay, bx, by);
    }
    if (m_check == CN_CHECK_LINE) {
        return line_ok(ax, ay, bx, by);
    }
    if (m_check == CN_CHECK_HYBRID) {
        if (line_ok(ax, ay, bx, by)) {
            return true;
        }
        return reach(ax, ay, bx, by);
    }
    return true;
}

bool CityNetwork::would_accept (u16 owner, u16 cand) const {
    const int rq = quad_of_xy(pos_x(owner), pos_y(owner), pos_x(cand), pos_y(cand));
    if (rq < 0) {
        return false;
    }
    const CityNetLinks& L = m_links[owner];
    u16 cur = U16_KEY_NULL;
    if (rq == 0) {
        cur = L.m_ne;
    } else if (rq == 1) {
        cur = L.m_nw;
    } else if (rq == 2) {
        cur = L.m_se;
    } else {
        cur = L.m_sw;
    }
    if (cur == U16_KEY_NULL || cur == cand) {
        return true;
    }
    const u16 ox = pos_x(owner);
    const u16 oy = pos_y(owner);
    return dist2_xy(ox, oy, pos_x(cand), pos_y(cand)) < dist2_xy(ox, oy, pos_x(cur), pos_y(cur));
}

u16 CityNetwork::find_best (u16 src, int q, bool need_accept) {
    if (q < 0 || m_masses == nullptr || m_cell_head == nullptr || m_mass == nullptr) {
        return U16_KEY_NULL;
    }
    const u16 mi = m_masses[src];
    if (mi == static_cast<u16>(LAND_MASS_IDX_NONE)) {
        return U16_KEY_NULL;
    }
    const u16 sx = pos_x(src);
    const u16 sy = pos_y(src);
    i32 x0 = 0;
    i32 x1 = 0;
    i32 y0 = 0;
    i32 y1 = 0;
    if (!quad_aabb(sx, sy, q, m_mass->m_w, m_mass->m_h, &x0, &x1, &y0, &y1)) {
        return U16_KEY_NULL;
    }
    const i32 cx0 = x0 / static_cast<i32>(CN_CELL);
    const i32 cx1 = x1 / static_cast<i32>(CN_CELL);
    const i32 cy0 = y0 / static_cast<i32>(CN_CELL);
    const i32 cy1 = y1 / static_cast<i32>(CN_CELL);
    static const u32 G_CAND_MAX = 128u;
    u16 ids[G_CAND_MAX];
    u32 ds[G_CAND_MAX];
    u32 cn = 0;
    for (i32 cy = cy0; cy <= cy1; ++cy) {
        for (i32 cx = cx0; cx <= cx1; ++cx) {
            const u32 ci = static_cast<u32>(cy) * static_cast<u32>(m_gw) + static_cast<u32>(cx);
            u16 j = m_cell_head[ci];
            while (j != U16_KEY_NULL) {
                if (j != src && m_masses[j] == mi && quad_of_xy(sx, sy, pos_x(j), pos_y(j)) == q) {
                    if (!need_accept || would_accept(j, src)) {
                        const u32 d = dist2_xy(sx, sy, pos_x(j), pos_y(j));
                        if (d <= G_LINK_RANGE2 && cn < G_CAND_MAX) {
                            ids[cn] = j;
                            ds[cn] = d;
                            ++cn;
                        }
                    }
                }
                j = m_cell_next[j];
            }
        }
    }
    for (u32 a = 1; a < cn; ++a) {
        const u16 id = ids[a];
        const u32 d = ds[a];
        u32 b = a;
        while (b > 0u && ds[b - 1u] > d) {
            ids[b] = ids[b - 1u];
            ds[b] = ds[b - 1u];
            --b;
        }
        ids[b] = id;
        ds[b] = d;
    }
    for (u32 i = 0; i < cn; ++i) {
        if (pair_ok(src, ids[i])) {
            return ids[i];
        }
    }
    return U16_KEY_NULL;
}

void CityNetwork::enq_repair (u16 i, int q) {
    if (q < 0 || i >= m_city_n || m_rep_id == nullptr) {
        return;
    }
    if (m_rep_n >= m_rep_cap) {
        return;
    }
    m_rep_id[m_rep_n] = i;
    m_rep_q[m_rep_n] = static_cast<u8>(q);
    ++m_rep_n;
}

void CityNetwork::bind_pair (u16 a, u16 b) {
    const int qa = quad_of_xy(pos_x(a), pos_y(a), pos_x(b), pos_y(b));
    const int qb = quad_of_xy(pos_x(b), pos_y(b), pos_x(a), pos_y(a));
    if (qa < 0 || qb < 0) {
        return;
    }
    u16* sa = link_slot(m_links[a], qa);
    u16* sb = link_slot(m_links[b], qb);
    if (*sa == b && *sb == a) {
        return;
    }
    const u16 old_a = *sa;
    const u16 old_b = *sb;
    if (old_a != U16_KEY_NULL && old_a != b) {
        const int oq = quad_of_xy(pos_x(old_a), pos_y(old_a), pos_x(a), pos_y(a));
        if (oq >= 0) {
            u16* os = link_slot(m_links[old_a], oq);
            if (*os == a) {
                *os = U16_KEY_NULL;
                enq_repair(old_a, oq);
            }
        }
    }
    if (old_b != U16_KEY_NULL && old_b != a) {
        const int oq = quad_of_xy(pos_x(old_b), pos_y(old_b), pos_x(b), pos_y(b));
        if (oq >= 0) {
            u16* os = link_slot(m_links[old_b], oq);
            if (*os == b) {
                *os = U16_KEY_NULL;
                enq_repair(old_b, oq);
            }
        }
    }
    *sa = b;
    *sb = a;
}

void CityNetwork::drain_repairs () {
    if (m_rep_id == nullptr) {
        return;
    }
    u32 head = 0;
    u32 guard = 0;
    const u32 guard_max = static_cast<u32>(m_city_n) * 16u + 64u;
    while (head < m_rep_n && guard < guard_max) {
        ++guard;
        const u16 i = m_rep_id[head];
        const int q = static_cast<int>(m_rep_q[head]);
        ++head;
        if (i >= m_city_n || q < 0) {
            continue;
        }
        u16* slot = link_slot(m_links[i], q);
        if (*slot != U16_KEY_NULL) {
            continue;
        }
        const u16 best = find_best(i, q, true);
        if (best == U16_KEY_NULL) {
            continue;
        }
        bind_pair(i, best);
    }
    m_rep_n = 0;
}

void CityNetwork::try_link (u16 src, u16 cand, int q) {
    if (q < 0) {
        return;
    }
    const u32 d = dist2_xy(pos_x(src), pos_y(src), pos_x(cand), pos_y(cand));
    if (d > G_LINK_RANGE2) {
        return;
    }
    u16* slot = link_slot(m_links[src], q);
    if (*slot != U16_KEY_NULL) {
        if (dist2_xy(pos_x(src), pos_y(src), pos_x(*slot), pos_y(*slot)) <= d) {
            return;
        }
    }
    if (!pair_ok(src, cand)) {
        return;
    }
    m_rep_n = 0;
    bind_pair(src, cand);
    drain_repairs();
}

bool CityNetwork::begin (
    CityArray& cities, const LandMassIndexRslt& mass, const u8* terr, u16 tw, u16 th, CnCheckMode check) {
    clear();
    if (mass.m_ov == nullptr || mass.m_w == 0 || mass.m_h == 0) {
        return false;
    }
    if (check != CN_CHECK_OFF && (terr == nullptr || tw == 0 || th == 0)) {
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
    m_links = new CityNetLinks[cap];
    m_masses = new u16[cap];
    m_rep_cap = 65536u * 2u + 64u;
    m_rep_id = new u16[m_rep_cap];
    m_rep_q = new u8[m_rep_cap];
    m_rep_n = 0;
    m_cap = cap;
    m_city_n = 0;
    m_cities = &cities;
    m_mass = &mass;
    m_terr = terr;
    m_tw = tw;
    m_th = th;
    m_check = check;
    for (u16 i = 0; i < cap; ++i) {
        clr_links(m_links[i]);
        m_masses[i] = static_cast<u16>(LAND_MASS_IDX_NONE);
    }
    init_grid(mass.m_w, mass.m_h);
    m_ok = true;
    return true;
}

bool CityNetwork::add (u16 city_idx) {
    if (!m_ok || m_cities == nullptr || m_mass == nullptr || m_cell_head == nullptr) {
        return false;
    }
    if (city_idx != m_city_n || city_idx >= m_cap) {
        return false;
    }
    const City* c = m_cities->get_city(city_idx);
    if (c == nullptr) {
        return false;
    }
    const u16 ni = city_idx;
    clr_links(m_links[ni]);
    m_masses[ni] = mass_at(*m_mass, c->get_x(), c->get_y());
    ++m_city_n;
    grid_ins(ni);
    const u16 mi = m_masses[ni];
    if (mi == static_cast<u16>(LAND_MASS_IDX_NONE)) {
        return true;
    }
    for (int q = 0; q < 4; ++q) {
        const u16 best = find_best(ni, q, false);
        if (best == U16_KEY_NULL) {
            continue;
        }
        m_rep_n = 0;
        bind_pair(ni, best);
        drain_repairs();
    }
    const i32 R = static_cast<i32>(CN_LINK_RANGE);
    const i32 x = static_cast<i32>(c->get_x());
    const i32 y = static_cast<i32>(c->get_y());
    const i32 mx = static_cast<i32>(m_mass->m_w) - 1;
    const i32 my = static_cast<i32>(m_mass->m_h) - 1;
    const i32 x0 = (x - R > 0) ? (x - R) : 0;
    const i32 x1 = (x + R < mx) ? (x + R) : mx;
    const i32 y0 = (y - R > 0) ? (y - R) : 0;
    const i32 y1 = (y + R < my) ? (y + R) : my;
    const i32 cx0 = x0 / static_cast<i32>(CN_CELL);
    const i32 cx1 = x1 / static_cast<i32>(CN_CELL);
    const i32 cy0 = y0 / static_cast<i32>(CN_CELL);
    const i32 cy1 = y1 / static_cast<i32>(CN_CELL);
    for (i32 cy = cy0; cy <= cy1; ++cy) {
        for (i32 cx = cx0; cx <= cx1; ++cx) {
            const u32 ci = static_cast<u32>(cy) * static_cast<u32>(m_gw) + static_cast<u32>(cx);
            u16 j = m_cell_head[ci];
            while (j != U16_KEY_NULL) {
                if (j != ni && m_masses[j] == mi) {
                    try_link(j, ni, quad_of_xy(pos_x(j), pos_y(j), pos_x(ni), pos_y(ni)));
                }
                j = m_cell_next[j];
            }
        }
    }
    return true;
}

bool CityNetwork::is_valid () const {
    return m_ok;
}

u16 CityNetwork::city_n () const {
    return m_city_n;
}

u32 CityNetwork::line_n () const {
    return m_line_n;
}

u32 CityNetwork::flood_n () const {
    return m_flood_n;
}

const CityArray* CityNetwork::cities () const {
    return m_cities;
}

const CityNetLinks* CityNetwork::links () const {
    return m_links;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
