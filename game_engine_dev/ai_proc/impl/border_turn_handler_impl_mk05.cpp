//================================================================================================================================
//=> - Includes (mk05: flood from capital, N nearest foreign players, own city nearest contact) -
//================================================================================================================================

#include <cstring>

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"
#include "game_map_defs.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Locals (mk05) -
//================================================================================================================================

static const i8 k_dx[8] = {0, 1, 1, 1, 0, -1, -1, -1};
static const i8 k_dy[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
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

static bool bth_walk_ok (const GameArraySimple& map, u16 x, u16 y) {
    const u8 t = map.get_terrain(x, y);
    if (t == TERR_OCEAN[0] || t == TERR_SEA[0] || t == TERR_COASTAL[0]) {
        return false;
    }
    if (t == TERR_INLAND_SEA[0] || t == TERR_INLAND_LAKE[0]) {
        return false;
    }
    if (t == TERR_MOUNTAINS[0] || t == TERR_VOLCANO[0]) {
        return false;
    }
    return true;
}

static bool bth_wb_ok (u16 w, u16 h) {
    if (WhiteboardMng::width() == w && WhiteboardMng::height() == h) {
        return true;
    }
    if (WhiteboardMng::chkout() != 0u) {
        return false;
    }
    if (WhiteboardMng::width() != 0u || WhiteboardMng::height() != 0u) {
        WhiteboardMng::terminate();
    }
    WhiteboardMng::init(w, h);
    return WhiteboardMng::width() == w && WhiteboardMng::height() == h;
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
        const u32 adx = tx > c->get_x()
            ? static_cast<u32>(tx - c->get_x()) : static_cast<u32>(c->get_x() - tx);
        const u32 ady = ty > c->get_y()
            ? static_cast<u32>(ty - c->get_y()) : static_cast<u32>(c->get_y() - ty);
        const u32 d = adx + ady;
        if (d < best_d) {
            best_d = d;
            best = i;
        }
    }
    return best;
}

//================================================================================================================================
//=> - BorderTurnHandler::handle (mk05) -
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
    const u16 w = m_map->width();
    const u16 h = m_map->height();
    if (cx >= w || cy >= h || !bth_walk_ok(*m_map, cx, cy) || !bth_wb_ok(w, h)) {
        fallback_cap(fronts, n, m_cap);
        sync_from(fronts, n);
        return true;
    }
    Whiteboard_1B seen("BorderTurnHandler", "mk05_seen", 0u);
    Whiteboard_2B cit("BorderTurnHandler", "mk05_cit", 0u);
    Whiteboard_4B que("BorderTurnHandler", "mk05_que", 0u);
    if (!seen.ok() || !cit.ok() || !que.ok()) {
        fallback_cap(fronts, n, m_cap);
        sync_from(fronts, n);
        return true;
    }
    const u32 tn = m_map->tile_n();
    std::memset(seen.raw(), 0, static_cast<size_t>(tn));
    for (u32 i = 0; i < tn; ++i) {
        cit.wr_i(i, U16_KEY_NULL);
    }
    const u16 cn = m_cities->get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_cities->get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 x = c->get_x();
        const u16 y = c->get_y();
        if (x >= w || y >= h) {
            continue;
        }
        cit.wr(x, y, i);
    }
    u8 hit_pl[k_pl_cap];
    std::memset(hit_pl, 0, sizeof(hit_pl));
    for (u16 u = 0; u < used_n; ++u) {
        const City* uc = m_cities->get_city(used[u]);
        if (uc == nullptr) {
            continue;
        }
        const u16 ux = uc->get_x();
        const u16 uy = uc->get_y();
        u16 best_ow = U16_KEY_NULL;
        u32 best_d = 0xFFFFFFFFu;
        for (u16 i = 0; i < cn; ++i) {
            const City* fc = m_cities->get_city(i);
            if (fc == nullptr) {
                continue;
            }
            const u16 ow = fc->get_owner();
            if (ow == seat || ow == U16_KEY_NULL || ow >= k_pl_cap || hit_pl[ow] != 0u) {
                continue;
            }
            const u32 adx = ux > fc->get_x()
                ? static_cast<u32>(ux - fc->get_x()) : static_cast<u32>(fc->get_x() - ux);
            const u32 ady = uy > fc->get_y()
                ? static_cast<u32>(uy - fc->get_y()) : static_cast<u32>(fc->get_y() - uy);
            const u32 d = adx + ady;
            if (d < best_d) {
                best_d = d;
                best_ow = ow;
            }
        }
        if (best_ow != U16_KEY_NULL) {
            hit_pl[best_ow] = 1u;
        }
    }
    u16 hit_x[k_front_cap];
    u16 hit_y[k_front_cap];
    u16 hit_n = 0u;
    u32 qh = 0u;
    u32 qt = 0u;
    que.wr_i(qt++, static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx));
    seen.wr(cx, cy, 1u);
    while (qh < qt && hit_n < need) {
        const u32 i = que.rd_i(qh++);
        const u16 x = static_cast<u16>(i % static_cast<u32>(w));
        const u16 y = static_cast<u16>(i / static_cast<u32>(w));
        const u16 cidx = cit.rd(x, y);
        if (cidx != U16_KEY_NULL) {
            const City* c = m_cities->get_city(cidx);
            if (c != nullptr) {
                const u16 ow = c->get_owner();
                if (ow != seat && ow != U16_KEY_NULL && ow < k_pl_cap && hit_pl[ow] == 0u) {
                    hit_pl[ow] = 1u;
                    hit_x[hit_n] = x;
                    hit_y[hit_n] = y;
                    hit_n = static_cast<u16>(hit_n + 1u);
                }
            }
        }
        for (u8 d = 0; d < 8u; ++d) {
            const i32 nx = static_cast<i32>(x) + static_cast<i32>(k_dx[d]);
            const i32 ny = static_cast<i32>(y) + static_cast<i32>(k_dy[d]);
            if (nx < 0 || ny < 0 || nx >= static_cast<i32>(w) || ny >= static_cast<i32>(h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(nx);
            const u16 uy = static_cast<u16>(ny);
            if (seen.rd(ux, uy) != 0u || !bth_walk_ok(*m_map, ux, uy)) {
                continue;
            }
            seen.wr(ux, uy, 1u);
            que.wr_i(qt++, static_cast<u32>(uy) * static_cast<u32>(w) + static_cast<u32>(ux));
        }
    }
    BorderFront picks[k_front_cap];
    u16 pick_n = 0u;
    for (u16 hix = 0; hix < hit_n && pick_n < need; ++hix) {
        const u16 oi = bth_own_near(*m_cities, seat, hit_x[hix], hit_y[hix], used, used_n);
        if (oi == U16_KEY_NULL) {
            continue;
        }
        used[used_n] = oi;
        used_n = static_cast<u16>(used_n + 1u);
        picks[pick_n].m_city = oi;
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
