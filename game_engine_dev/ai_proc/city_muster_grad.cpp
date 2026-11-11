//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_muster_grad.h"

#include "city.h"
#include "city_array.h"
#include "generate_distance_p2p.h"
#include "game_state.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - CityMusterGrad -
//================================================================================================================================

CityMusterGrad::CityMusterGrad () : m_st(nullptr), m_ok(false) {
}

CityMusterGrad::~CityMusterGrad () {
    clr();
}

void CityMusterGrad::clr () {
    m_st = nullptr;
    m_ok = false;
}

bool CityMusterGrad::bind (GameState* st) {
    clr();
    if (st == nullptr || st->m_statics == nullptr || st->m_player_states == nullptr || st->m_player_n == 0u) {
        return false;
    }
    if (!m_board.ok()) {
        return false;
    }
    m_st = st;
    return true;
}

bool CityMusterGrad::ok () const {
    return m_ok && m_st != nullptr && m_board.ok();
}

const WalkP2P& CityMusterGrad::board () const {
    return m_board;
}

u16 CityMusterGrad::muster_city (u16 seat) const {
    if (m_st == nullptr || seat >= m_st->m_player_n || m_st->m_player_states == nullptr) {
        return U16_KEY_NULL;
    }
    return m_st->m_player_states[seat].m_muster_city;
}

bool CityMusterGrad::set_caps () {
    if (m_st == nullptr || m_st->m_player_states == nullptr) {
        return false;
    }
    for (u16 s = 0; s < m_st->m_player_n; ++s) {
        m_st->m_player_states[s].m_muster_city = U16_KEY_NULL;
    }
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        const City* c = m_st->m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 own = c->get_owner();
        if (own >= m_st->m_player_n) {
            continue;
        }
        if (m_st->m_player_states[own].m_muster_city == U16_KEY_NULL) {
            m_st->m_player_states[own].m_muster_city = i;
        }
    }
    return true;
}

void CityMusterGrad::init_sent () {
    u16* turn = m_board.turn();
    u16* rem = m_board.rem();
    if (turn == nullptr || rem == nullptr) {
        return;
    }
    const u32 n = m_st->m_map.tile_n();
    for (u32 i = 0; i < n; ++i) {
        turn[i] = GenerateDistanceP2P::k_turn_sent;
        rem[i] = 0u;
    }
}

void CityMusterGrad::mark_flood (u16 seat) {
    if (m_st == nullptr) {
        return;
    }
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = m_st->m_cities.get_city(i);
        if (c == nullptr || c->get_owner() != seat) {
            continue;
        }
        c->set_new_muster_flood(true);
        c->set_in_muster_reach(false);
    }
}

bool CityMusterGrad::flood_seat (u16 seat) {
    if (m_st == nullptr || seat >= m_st->m_player_n) {
        return false;
    }
    mark_flood(seat);
    const u16 ci = m_st->m_player_states[seat].m_muster_city;
    if (ci == U16_KEY_NULL) {
        return true;
    }
    const City* c = m_st->m_cities.get_city(ci);
    if (c == nullptr || c->get_owner() != seat) {
        m_st->m_player_states[seat].m_muster_city = U16_KEY_NULL;
        return true;
    }
    u16 dmax = 0u;
    if (!GenerateDistanceP2P::generate(
            *m_st, *m_st->m_statics, c->get_x(), c->get_y(), m_board, seat,
            GenerateDistanceP2P::k_acc_own, &dmax)) {
        m_st->m_player_states[seat].m_muster_city = U16_KEY_NULL;
        return true;
    }
    return true;
}

bool CityMusterGrad::city_reach (u16 city_idx) {
    if (m_st == nullptr) {
        return false;
    }
    City* c = m_st->m_cities.get_city(city_idx);
    if (c == nullptr) {
        return false;
    }
    if (c->new_muster_flood()) {
        c->set_new_muster_flood(false);
        c->set_in_muster_reach(walk_muster(c->get_x(), c->get_y()));
    }
    return c->in_muster_reach();
}

bool CityMusterGrad::flood (u16 seat) {
    if (m_st == nullptr || m_st->m_statics == nullptr || !m_board.ok()) {
        return false;
    }
    if (WhiteboardMng::width() != m_st->m_map.width() || WhiteboardMng::height() != m_st->m_map.height()) {
        return false;
    }
    if (seat >= m_st->m_player_n || m_st->m_player_states[seat].m_is_active == 0u) {
        return false;
    }
    if (!flood_seat(seat)) {
        m_ok = false;
        return false;
    }
    m_ok = true;
    return true;
}

bool CityMusterGrad::walk_muster (u16 x, u16 y) const {
    if (!ok() || m_st == nullptr || m_st->m_player_states == nullptr) {
        return false;
    }
    const u16 w = m_st->m_map.width();
    const u16 h = m_st->m_map.height();
    if (x >= w || y >= h) {
        return false;
    }
    const u8 own = m_st->m_map.get_civ_owner(x, y);
    if (own == U8_KEY_NULL || static_cast<u16>(own) >= m_st->m_player_n) {
        return false;
    }
    const u16 mi = m_st->m_player_states[own].m_muster_city;
    if (mi == U16_KEY_NULL) {
        return false;
    }
    const City* m = m_st->m_cities.get_city(mi);
    if (m == nullptr) {
        return false;
    }
    const u16 mx = m->get_x();
    const u16 my = m->get_y();
    const u16* turn = m_board.turn();
    if (turn == nullptr) {
        return false;
    }
    const u32 start = static_cast<u32>(y) * static_cast<u32>(w) + static_cast<u32>(x);
    if (turn[start] == GenerateDistanceP2P::k_turn_sent) {
        return false;
    }
    u16 cx = x;
    u16 cy = y;
    const u32 step_cap = m_st->m_map.tile_n() + 1u;
    for (u32 s = 0; s < step_cap; ++s) {
        if (cx == mx && cy == my) {
            return true;
        }
        const u32 i = static_cast<u32>(cy) * static_cast<u32>(w) + static_cast<u32>(cx);
        if (turn[i] == 0u) {
            return true;
        }
        const WalkP2P::StepRes step = m_board.peek(*m_st, cx, cy);
        if (!step.have) {
            return false;
        }
        cx = step.nx;
        cy = step.ny;
    }
    return false;
}

bool CityMusterGrad::build () {
    m_ok = false;
    if (m_st == nullptr || m_st->m_statics == nullptr || !m_board.ok()) {
        return false;
    }
    if (WhiteboardMng::width() != m_st->m_map.width() || WhiteboardMng::height() != m_st->m_map.height()) {
        return false;
    }
    init_sent();
    for (u16 s = 0; s < m_st->m_player_n; ++s) {
        if (m_st->m_player_states[s].m_is_active == 0u) {
            continue;
        }
        if (!flood_seat(s)) {
            return false;
        }
    }
    m_ok = true;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
