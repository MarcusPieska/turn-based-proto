//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_muster_grad.h"

#include "build_adds_array.h"
#include "city.h"
#include "city_array.h"
#include "generate_distance_p2p.h"
#include "game_state.h"
#include "log_dbg.h"
#include "lucky_seat_turn.h"
#include "runtime_statics.h"
#include "unit_movement_mng.h"
#include "unit_static_key.h"
#include "unit_utility_helper.h"
#include "whiteboard_mng.h"

//================================================================================================================================
//=> - Static loop instance -
//================================================================================================================================

static CityMusterGrad* g_cm = nullptr;

static const unsigned k_mpull_feed_peel = 1u;
static const unsigned k_mpull_feed_arr_cap = 2u;

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
    const u16 seat = LuckySeatTurn::at(*m_st, m_st->m_current_turn);
    if (seat != U16_KEY_NULL && !flood_seat(seat)) {
        return false;
    }
    m_ok = true;
    return true;
}

bool CityMusterGrad::feed_unit (const UnitAddStruct& u) const {
    if (m_st == nullptr || m_st->m_statics == nullptr) {
        return false;
    }
    if (u.m_in_campaign != 0u || u.m_in_army != 0u || u.m_being_extended != 0u) {
        return false;
    }
    if (u.m_x == U16_KEY_NULL || u.m_y == U16_KEY_NULL) {
        return false;
    }
    const u16 typ_n = m_st->m_statics->unit().get_item_count();
    if (u.m_unit_typ_idx >= typ_n) {
        return false;
    }
    const u16 ut = m_st->m_statics->unit().get_item(UnitStaticDataKey::from_raw(u.m_unit_typ_idx)).type;
    return UnitUtilityHelper::is_type_for_land_army(ut);
}

bool CityMusterGrad::step_one (UnitAddKey key) {
    if (!ok() || !key.is_valid()) {
        return false;
    }
    UnitAddStruct* u = m_st->m_units.get_unit_add(key);
    if (u == nullptr || !feed_unit(*u)) {
        return false;
    }
    const u16 seat = u->m_player_idx;
    const u16 mi = muster_city(seat);
    if (mi == U16_KEY_NULL) {
        return false;
    }
    const City* m = m_st->m_cities.get_city(mi);
    if (m == nullptr) {
        return false;
    }
    if (u->m_x == m->get_x() && u->m_y == m->get_y()) {
        return false;
    }
    const u16* turn = m_board.turn();
    if (turn == nullptr) {
        return false;
    }
    const u16 w = m_st->m_map.width();
    const u32 i = static_cast<u32>(u->m_y) * static_cast<u32>(w) + static_cast<u32>(u->m_x);
    if (turn[i] == GenerateDistanceP2P::k_turn_sent) {
        return false;
    }
    bool any = false;
    for (;;) {
        u = m_st->m_units.get_unit_add(key);
        if (u == nullptr || u->m_x == U16_KEY_NULL) {
            break;
        }
        if (u->m_x == m->get_x() && u->m_y == m->get_y()) {
            break;
        }
        const WalkP2P::StepRes step = m_board.peek(*m_st, u->m_x, u->m_y);
        if (!step.have) {
            break;
        }
        if (!UnitMovementMng::can_step(*m_st, key, step.nx, step.ny, nullptr)) {
            break;
        }
        if (!UnitMovementMng::apply_step(*m_st, key, step.nx, step.ny)) {
            break;
        }
        any = true;
        u = m_st->m_units.get_unit_add(key);
        if (u != nullptr && u->m_x == m->get_x() && u->m_y == m->get_y()) {
            LOG_WAR_MUSTER_PULL::LOG(
                SC_U32(seat), SC_U32(m->get_x()), SC_U32(m->get_y()),
                k_mpull_feed_arr_cap, SC_U32(m_st->m_current_turn));
            break;
        }
    }
    return any;
}

void CityMusterGrad::pull_cities () {
    if (!ok()) {
        return;
    }
    const u16 cn = m_st->m_cities.get_city_count();
    for (u16 i = 0; i < cn; ++i) {
        City* c = m_st->m_cities.get_city(i);
        if (c == nullptr) {
            continue;
        }
        const u16 seat = c->get_owner();
        if (seat >= m_st->m_player_n || m_st->m_player_states[seat].m_is_active == 0u) {
            continue;
        }
        const u16 mi = muster_city(seat);
        if (mi == U16_KEY_NULL || mi == i) {
            continue;
        }
        if (!city_reach(i)) {
            continue;
        }
        UnitAddKey head = UnitAddKey::None();
        if (!UnitMovementMng::muster_leave_one_defense(*m_st, c->get_x(), c->get_y(), seat, &head)) {
            continue;
        }
        UnitAddStruct* hu = m_st->m_units.get_unit_add(head);
        if (hu == nullptr || !feed_unit(*hu)) {
            continue;
        }
        LOG_WAR_MUSTER_PULL::LOG(
            SC_U32(seat), SC_U32(c->get_x()), SC_U32(c->get_y()),
            k_mpull_feed_peel, SC_U32(m_st->m_current_turn));
        (void)step_one(head);
    }
}

void CityMusterGrad::step_heads () {
    if (!ok()) {
        return;
    }
    const u32 scan_n = static_cast<u32>(m_st->m_units.get_head_unit_add_idx());
    for (u32 idx = 0; idx < scan_n; ++idx) {
        const UnitAddKey key = UnitAddKey::from_raw(static_cast<u16>(idx));
        const UnitAddStruct* u = m_st->m_units.get_unit_add(key);
        if (u == nullptr || !feed_unit(*u)) {
            continue;
        }
        if (m_st->m_map.get_add_typ(u->m_x, u->m_y) == BUILD_ADD_CITY) {
            const u16 ci = m_st->m_map.get_add_idx(u->m_x, u->m_y);
            const u16 mi = muster_city(u->m_player_idx);
            if (ci != mi) {
                continue;
            }
        }
        (void)step_one(key);
    }
}

bool CityMusterGrad::begin (GameState& st) {
    clear();
    g_cm = new CityMusterGrad();
    if (!g_cm->bind(&st) || !g_cm->set_caps() || !g_cm->build()) {
        clear();
        return false;
    }
    return true;
}

void CityMusterGrad::clear () {
    if (g_cm != nullptr) {
        delete g_cm;
        g_cm = nullptr;
    }
}

void CityMusterGrad::handle (GameState& st) {
    if (g_cm == nullptr || g_cm->m_st != &st) {
        return;
    }
    if (!g_cm->set_caps() || !g_cm->build()) {
        g_cm->m_ok = false;
        return;
    }
    g_cm->pull_cities();
    g_cm->step_heads();
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
