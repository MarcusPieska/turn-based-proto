//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "walk_river_mk1.h"
#include "river_pathing.h"
#include "runtime_trace_dbg.h"

//================================================================================================================================
//=> - WalkRiverMk1 static helpers -
//================================================================================================================================

u16 WalkRiverMk1::can_ai_start_from (
    const GameArraySimple& map,
    MapBitOverlay& exp,
    u16 x,
    u16 y,
    u16 sight) {
    if (RiverPathing::is_riv(map, x, y)) {
        u16 ox = 0u;
        u16 oy = 0u;
        if (RiverPathing::pick_near_front(map, exp, x, y, sight, ox, oy, 0u, 0u, false)) {
            return 1u;
        }
    }
    PathMk1 path;
    if (RiverPathing::find_land_path_to_river(map, exp, x, y, sight, path)) {
        return 2u;
    }
    return 0u;
}

//================================================================================================================================
//=> - WalkRiverMk1 -
//================================================================================================================================

WalkRiverMk1::WalkRiverMk1 (const GameArraySimple& map, MapBitOverlay& explored, u16 sx, u16 sy, u16 sight, u8 player) :
    m_map(map),
    m_exp(explored),
    m_pos{sx, sy},
    m_sp{sx, sy},
    m_sight(sight),
    m_player(player),
    m_ph(k_ph_done),
    m_path_i(0),
    m_greedy_ph(false),
    m_has_blk(false),
    m_blk{0, 0},
    m_land_approach(false) {
    const u16 st = can_ai_start_from(map, explored, sx, sy, sight);
    if (st == 2u) {
        if (RiverPathing::find_land_path_to_river(map, explored, sx, sy, sight, m_path)) {
            m_ph = k_ph_path;
            m_land_approach = true;
            m_path_i = 0u;
            while (m_path_i < m_path.n()
                && m_path.x(m_path_i) == m_pos.x && m_path.y(m_path_i) == m_pos.y) {
                ++m_path_i;
            }
            if (m_path_i >= m_path.n()) {
                end_all();
            }
        }
    } else if (st == 1u) {
        start_greedy_ph();
    }
    reveal_around();
}

void WalkRiverMk1::start_greedy_ph () {
    m_ph = k_ph_greedy;
    m_greedy_ph = true;
    m_has_blk = false;
    m_mk_gre_start.push(m_pos.x, m_pos.y);
}

void WalkRiverMk1::end_greedy_local () {
    m_greedy_ph = false;
    m_has_blk = false;
    m_mk_gre_end.push(m_pos.x, m_pos.y);
    m_path.clr();
    m_path_i = 0;
    if (!RiverPathing::find_path_to_front(m_map, m_exp, m_pos.x, m_pos.y, m_sight, m_path)) {
        end_all();
        return;
    }
    while (m_path_i < m_path.n()
        && m_path.x(m_path_i) == m_pos.x && m_path.y(m_path_i) == m_pos.y) {
        ++m_path_i;
    }
    if (m_path_i >= m_path.n()) {
        end_all();
        return;
    }
    m_ph = k_ph_path;
}

void WalkRiverMk1::end_all () {
    m_greedy_ph = false;
    m_has_blk = false;
    m_ph = k_ph_done;
}

void WalkRiverMk1::reveal_around () {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(m_pos.x) + dx;
            const i32 yi = static_cast<i32>(m_pos.y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h || m_exp.get(x, y) != 0u) {
                continue;
            }
            m_exp.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(m_player)));
        }
    }
}

void WalkRiverMk1::reveal_delta (MapPt o, MapPt n) {
    const u16 w = m_map.width();
    const u16 h = m_map.height();
    for (i16 dy = -static_cast<i16>(m_sight); dy <= static_cast<i16>(m_sight); ++dy) {
        for (i16 dx = -static_cast<i16>(m_sight); dx <= static_cast<i16>(m_sight); ++dx) {
            const i32 adx = (dx < 0) ? -dx : dx;
            const i32 ady = (dy < 0) ? -dy : dy;
            if (static_cast<u32>((adx > ady) ? adx : ady) > static_cast<u32>(m_sight)) {
                continue;
            }
            const i32 xi = static_cast<i32>(n.x) + dx;
            const i32 yi = static_cast<i32>(n.y) + dy;
            if (xi < 0 || yi < 0) {
                continue;
            }
            const u16 x = static_cast<u16>(xi);
            const u16 y = static_cast<u16>(yi);
            if (x >= w || y >= h) {
                continue;
            }
            const i32 oxo = static_cast<i32>(x) - static_cast<i32>(o.x);
            const i32 oyo = static_cast<i32>(y) - static_cast<i32>(o.y);
            const i32 oax = (oxo < 0) ? -oxo : oxo;
            const i32 oay = (oyo < 0) ? -oyo : oyo;
            if (static_cast<u32>((oax > oay) ? oax : oay) <= static_cast<u32>(m_sight)) {
                continue;
            }
            if (m_exp.get(x, y) != 0u) {
                continue;
            }
            m_exp.set(x, y);
            TRACE_EXPLORE_DISCOVER((x, y, static_cast<u16>(m_player)));
        }
    }
}

bool WalkRiverMk1::step_greedy () {
    if (!m_greedy_ph) {
        return false;
    }
    u16 nx = 0;
    u16 ny = 0;
    if (RiverPathing::pick_near_front(m_map, m_exp, m_pos.x, m_pos.y, m_sight, nx, ny,
            m_blk.x, m_blk.y, m_has_blk)) {
        const u16 ox = m_pos.x;
        const u16 oy = m_pos.y;
        m_blk.x = ox;
        m_blk.y = oy;
        m_has_blk = true;
        m_pos.x = nx;
        m_pos.y = ny;
        reveal_delta({ox, oy}, m_pos);
        return true;
    }
    end_greedy_local();
    if (m_ph == k_ph_path) {
        return step_path();
    }
    return false;
}

bool WalkRiverMk1::step_path () {
    if (m_path_i >= m_path.n()) {
        start_greedy_ph();
        return step_greedy();
    }
    while (m_path_i < m_path.n()
        && m_path.x(m_path_i) == m_pos.x && m_path.y(m_path_i) == m_pos.y) {
        ++m_path_i;
    }
    if (m_path_i >= m_path.n()) {
        start_greedy_ph();
        return step_greedy();
    }
    const u16 tx = m_path.x(m_path_i);
    const u16 ty = m_path.y(m_path_i);
    if (!m_land_approach && !RiverPathing::is_riv(m_map, tx, ty)) {
        end_all();
        return false;
    }
    const u16 ox = m_pos.x;
    const u16 oy = m_pos.y;
    m_pos.x = tx;
    m_pos.y = ty;
    ++m_path_i;
    reveal_delta({ox, oy}, m_pos);
    if (m_land_approach && RiverPathing::is_riv(m_map, m_pos.x, m_pos.y)) {
        m_land_approach = false;
        start_greedy_ph();
        return true;
    }
    if (m_path_i >= m_path.n()) {
        start_greedy_ph();
        return step_greedy();
    }
    return true;
}

void WalkRiverMk1::move (u16 moves) {
    if (m_ph == k_ph_done) {
        return;
    }
    reveal_around();
    for (u16 m = 0; m < moves; ++m) {
        if (m_ph == k_ph_done) {
            break;
        }
        if (m_ph == k_ph_path) {
            if (!step_path()) {
                continue;
            }
            continue;
        }
        if (!step_greedy()) {
            if (m_ph == k_ph_done) {
                break;
            }
            continue;
        }
    }
}

u8 WalkRiverMk1::phase () const {
    return m_ph;
}

u16 WalkRiverMk1::x () const {
    return m_pos.x;
}

u16 WalkRiverMk1::y () const {
    return m_pos.y;
}

u16 WalkRiverMk1::path_n () const {
    return m_path.n();
}

u16 WalkRiverMk1::path_i () const {
    return m_path_i;
}

u16 WalkRiverMk1::wp_x (u16 i) const {
    return m_path.x(i);
}

u16 WalkRiverMk1::wp_y (u16 i) const {
    return m_path.y(i);
}

u16 WalkRiverMk1::gre_end_n () const {
    return m_mk_gre_end.n();
}

u16 WalkRiverMk1::gre_end_x (u16 i) const {
    return m_mk_gre_end.x(i);
}

u16 WalkRiverMk1::gre_end_y (u16 i) const {
    return m_mk_gre_end.y(i);
}

u16 WalkRiverMk1::gre_start_n () const {
    return m_mk_gre_start.n();
}

u16 WalkRiverMk1::gre_start_x (u16 i) const {
    return m_mk_gre_start.x(i);
}

u16 WalkRiverMk1::gre_start_y (u16 i) const {
    return m_mk_gre_start.y(i);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
