//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "settler_mission_manager.h"

#include "assert_log.h"
#include "city_blocking_mask.h"
#include "game_array_simple.h"
#include "gen_settlement_order.h"

//================================================================================================================================
//=> - Local -
//================================================================================================================================

static const u16 k_half = static_cast<u16>(SMM_WIN / 2u);

static u16 cheb (u16 ax, u16 ay, u16 bx, u16 by) {
    const u16 dx = ax > bx ? static_cast<u16>(ax - bx) : static_cast<u16>(bx - ax);
    const u16 dy = ay > by ? static_cast<u16>(ay - by) : static_cast<u16>(by - ay);
    return dx > dy ? dx : dy;
}

//================================================================================================================================
//=> - SettlerMissionManager -
//================================================================================================================================

SettlerMissionManager::SettlerMissionManager () :
    m_fn(0),
    m_oi(0),
    m_w(0),
    m_h(0),
    m_ok(false),
    m_opp(false) {
    clr();
}

void SettlerMissionManager::opp (bool v) {
    m_opp = v;
}

void SettlerMissionManager::clr () {
    for (u16 i = 0; i < SMM_SLOT_N; ++i) {
        m_slot[i].m_pl = 0;
        m_slot[i].m_x = 0;
        m_slot[i].m_y = 0;
        m_slot[i].m_tx = 0;
        m_slot[i].m_ty = 0;
        m_slot[i].m_steps = 0;
        m_slot[i].m_pn = 0;
        m_slot[i].m_pi = 0;
        m_slot[i].m_on = 0;
        m_fs[i] = static_cast<u8>(i);
    }
    m_fn = static_cast<u8>(SMM_SLOT_N);
    m_oi = 0;
    m_ok = false;
}

bool SettlerMissionManager::begin (
    const SectorNetwork& net,
    const SectorNetworkRouter& rt,
    const u8* terr,
    u16 w,
    u16 h)
{
    clr();
    GAME_EXPECT(terr != nullptr, "SettlerMissionManager begin got nullptr terrain");
    GAME_EXPECT(w != 0 && h != 0, "SettlerMissionManager begin got empty dimensions");
    if (!wbeg(net, rt, terr, w, h)) {
        clr();
        return false;
    }
    m_w = w;
    m_h = h;
    m_ok = true;
    return true;
}

void SettlerMissionManager::punch (GameArraySimple& map) {
    const u16 w = map.width();
    const u16 h = map.height();
    for (u16 y = 0; y < h; ++y) {
        for (u16 x = 0; x < w; ++x) {
            if (map.get_planned_city(x, y) != 0u) {
                map.set_settler_blocked(x, y, 0u);
            }
        }
    }
}

bool SettlerMissionManager::ok () const {
    return m_ok;
}

u16 SettlerMissionManager::idle () const {
    return m_fn;
}

void SettlerMissionManager::rel (u16 s) {
    if (s >= SMM_SLOT_N || m_slot[s].m_on == 0 || m_fn >= static_cast<u8>(SMM_SLOT_N)) {
        return;
    }
    m_slot[s].m_on = 0;
    m_fs[m_fn] = static_cast<u8>(s);
    m_fn = static_cast<u8>(m_fn + 1u);
}

bool SettlerMissionManager::on (u16 s) const {
    return s < SMM_SLOT_N && m_slot[s].m_on != 0;
}

u16 SettlerMissionManager::pl (u16 s) const {
    if (s >= SMM_SLOT_N) {
        return 0;
    }
    return m_slot[s].m_pl;
}

u16 SettlerMissionManager::x (u16 s) const {
    if (s >= SMM_SLOT_N) {
        return 0;
    }
    return m_slot[s].m_x;
}

u16 SettlerMissionManager::y (u16 s) const {
    if (s >= SMM_SLOT_N) {
        return 0;
    }
    return m_slot[s].m_y;
}

u16 SettlerMissionManager::tx (u16 s) const {
    if (s >= SMM_SLOT_N) {
        return 0;
    }
    return m_slot[s].m_tx;
}

u16 SettlerMissionManager::ty (u16 s) const {
    if (s >= SMM_SLOT_N) {
        return 0;
    }
    return m_slot[s].m_ty;
}

bool SettlerMissionManager::taken (u16 x, u16 y, u16 skip) const {
    for (u16 i = 0; i < SMM_SLOT_N; ++i) {
        if (i == skip || m_slot[i].m_on == 0) {
            continue;
        }
        if (m_slot[i].m_tx == x && m_slot[i].m_ty == y) {
            return true;
        }
    }
    return false;
}

bool SettlerMissionManager::ok_site (const GameArraySimple& map, u16 x, u16 y, u16 skip) const {
    if (x >= m_w || y >= m_h) {
        return false;
    }
    if (map.get_planned_city(x, y) == 0u) {
        return false;
    }
    if (map.get_settler_blocked(x, y) != 0u) {
        return false;
    }
    return !taken(x, y, skip);
}

bool SettlerMissionManager::pick_loc (GameArraySimple& map, u16 s, u16 x0, u16 y0) {
    for (u16 r = 0; r <= k_half; ++r) {
        for (int dy = -static_cast<int>(k_half); dy <= static_cast<int>(k_half); ++dy) {
            for (int dx = -static_cast<int>(k_half); dx <= static_cast<int>(k_half); ++dx) {
                const int x = static_cast<int>(x0) + dx;
                const int y = static_cast<int>(y0) + dy;
                if (x < 0 || y < 0 || x >= static_cast<int>(m_w) || y >= static_cast<int>(m_h)) {
                    continue;
                }
                const u16 ux = static_cast<u16>(x);
                const u16 uy = static_cast<u16>(y);
                if (cheb(x0, y0, ux, uy) != r) {
                    continue;
                }
                if (!ok_site(map, ux, uy, s)) {
                    continue;
                }
                if (aim(s, x0, y0, ux, uy)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool SettlerMissionManager::pick_ord (
    GameArraySimple& map,
    const GenSettlementOrder& ord,
    u16 s,
    u16 pl,
    u16 x0,
    u16 y0)
{
    const u32 n = ord.n(pl);
    while (m_oi < n) {
        const SpgCoordPair h = ord.at(pl, m_oi);
        if (map.get_planned_city(h.x, h.y) != 0u) {
            break;
        }
        m_oi += 1u;
    }
    for (u32 i = m_oi; i < n; ++i) {
        const SpgCoordPair pt = ord.at(pl, i);
        if (!ok_site(map, pt.x, pt.y, s)) {
            continue;
        }
        if (aim(s, x0, y0, pt.x, pt.y)) {
            return true;
        }
    }
    return false;
}

void SettlerMissionManager::found (GameArraySimple& map, u16 s) {
    Slot& sl = m_slot[s];
    map.set_planned_city(sl.m_x, sl.m_y, 0u);
    CityBlockingMask::stamp(map, sl.m_x, sl.m_y);
    const u16 r = CityBlockingMask::m_r_max;
    const int cr = static_cast<int>(r);
    for (int dy = -cr; dy <= cr; ++dy) {
        for (int dx = -cr; dx <= cr; ++dx) {
            const int x = static_cast<int>(sl.m_x) + dx;
            const int y = static_cast<int>(sl.m_y) + dy;
            if (x < 0 || y < 0 || x >= static_cast<int>(m_w) || y >= static_cast<int>(m_h)) {
                continue;
            }
            const u16 ux = static_cast<u16>(x);
            const u16 uy = static_cast<u16>(y);
            if (map.get_settler_blocked(ux, uy) != 0u) {
                map.set_planned_city(ux, uy, 0u);
            }
        }
    }
    rel(s);
}

u16 SettlerMissionManager::asgn (
    GameArraySimple& map,
    const GenSettlementOrder& ord,
    u16 pl,
    u16 x,
    u16 y)
{
    GAME_EXPECT(m_ok, "SettlerMissionManager asgn called before begin");
    GAME_EXPECT(pl < ord.pn(), "SettlerMissionManager asgn player out of range");
    if (!m_ok || m_fn == 0) {
        return U16_KEY_NULL;
    }
    if (!m_opp && m_oi >= ord.n(pl)) {
        return U16_KEY_NULL;
    }
    m_fn = static_cast<u8>(m_fn - 1u);
    const u16 s = m_fs[m_fn];
    Slot& sl = m_slot[s];
    sl.m_pl = pl;
    sl.m_steps = 0;
    sl.m_on = 0;
    if (!(m_opp && pick_loc(map, s, x, y)) && !pick_ord(map, ord, s, pl, x, y)) {
        m_fs[m_fn] = static_cast<u8>(s);
        m_fn = static_cast<u8>(m_fn + 1u);
        return U16_KEY_NULL;
    }
    sl.m_on = 1;
    sl.m_pl = pl;
    sl.m_steps = 0;
    return s;
}

u8 SettlerMissionManager::step (GameArraySimple& map, u16 s) {
    GAME_EXPECT(m_ok, "SettlerMissionManager step called before begin");
    GAME_EXPECT(s < SMM_SLOT_N, "SettlerMissionManager step slot out of range");
    if (!m_ok || s >= SMM_SLOT_N || m_slot[s].m_on == 0) {
        return SMM_DROP;
    }
    Slot& sl = m_slot[s];
    if (map.get_planned_city(sl.m_x, sl.m_y) != 0u) {
        found(map, s);
        return SMM_FOUND;
    }
    if (sl.m_steps != 0u && (sl.m_steps % static_cast<u16>(SMM_SCAN)) == 0u) {
        pick_loc(map, s, sl.m_x, sl.m_y);
    }
    if (wdn(s) || (sl.m_x == sl.m_tx && sl.m_y == sl.m_ty)) {
        if (map.get_planned_city(sl.m_x, sl.m_y) != 0u) {
            found(map, s);
            return SMM_FOUND;
        }
        rel(s);
        return SMM_DROP;
    }
    if (!wgo(s)) {
        if (wdn(s)) {
            if (map.get_planned_city(sl.m_x, sl.m_y) != 0u) {
                found(map, s);
                return SMM_FOUND;
            }
        }
        rel(s);
        return SMM_DROP;
    }
    sl.m_steps = static_cast<u16>(sl.m_steps + 1u);
    if (map.get_planned_city(sl.m_x, sl.m_y) != 0u) {
        found(map, s);
        return SMM_FOUND;
    }
    return SMM_GO;
}

void SettlerMissionManager::drop (u16 s) {
    GAME_EXPECT(s < SMM_SLOT_N, "SettlerMissionManager drop slot out of range");
    rel(s);
}

#ifndef SETTLER_MISSION_MANAGER_IMPL
#define SETTLER_MISSION_MANAGER_IMPL "impl/settler_mission_manager_impl_mk02.cpp"
#endif

#include SETTLER_MISSION_MANAGER_IMPL

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
