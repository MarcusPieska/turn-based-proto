//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "empire_muster.h"

#include "city.h"
#include "city_array.h"
#include "city_muster_grad.h"
#include "game_state.h"
#include "runtime_statics.h"
#include "unit_add_struct.h"
#include "unit_add_vector.h"
#include "unit_movement_mng.h"
#include "unit_static_data.h"
#include "unit_static_key.h"
#include "unit_utility_helper.h"

//================================================================================================================================
//=> - Locals -
//================================================================================================================================

static u16 s_min_n = 10u;
static EmpireMuster s_emp[EmpireMuster::k_seat_cap];
static u16 s_n = 0u;
static GameState* s_st = nullptr;

//================================================================================================================================
//=> - EmpireMuster -
//================================================================================================================================

EmpireMuster::EmpireMuster () :
    m_st(nullptr),
    m_seat(U16_KEY_NULL),
    m_pt_n(1u),
    m_disp_n(1u),
    m_ok(false) {
}

EmpireMuster::~EmpireMuster () {
    clr();
}

bool EmpireMuster::setup (u16 min_n) {
    s_min_n = (min_n == 0u) ? 1u : min_n;
    return true;
}

u16 EmpireMuster::min_n () {
    return s_min_n;
}

bool EmpireMuster::begin (GameState& st) {
    clear();
    if (st.m_player_n == 0u || st.m_player_n > k_seat_cap) {
        return false;
    }
    s_st = &st;
    s_n = st.m_player_n;
    for (u16 i = 0u; i < s_n; ++i) {
        s_emp[i].bind(st, i);
    }
    return true;
}

void EmpireMuster::clear () {
    for (u16 i = 0u; i < s_n && i < k_seat_cap; ++i) {
        s_emp[i].clr();
    }
    s_n = 0u;
    s_st = nullptr;
}

void EmpireMuster::handle (GameState& st) {
    if (s_st != &st || s_n == 0u) {
        return;
    }
    for (u16 i = 0u; i < s_n; ++i) {
        if (!s_emp[i].ok()) {
            continue;
        }
        if (st.m_player_states == nullptr || st.m_player_states[i].m_is_active == 0u) {
            continue;
        }
        (void)s_emp[i].update_pathing();
        if (s_emp[i].disp().busy()) {
            (void)s_emp[i].disp().walk_tick(st, i);
            continue;
        }
        (void)s_emp[i].try_dispatch();
    }
}

EmpireMuster* EmpireMuster::at (u16 seat) {
    if (seat >= s_n || seat >= k_seat_cap) {
        return nullptr;
    }
    return &s_emp[seat];
}

void EmpireMuster::bind (GameState& st, u16 seat) {
    clr();
    m_st = &st;
    m_seat = seat;
    m_pt_n = 1u;
    m_disp_n = 1u;
    m_ok = (seat < st.m_player_n);
}

void EmpireMuster::clr () {
    m_disp.clr();
    m_st = nullptr;
    m_seat = U16_KEY_NULL;
    m_pt_n = 1u;
    m_disp_n = 1u;
    m_ok = false;
}

bool EmpireMuster::ok () const {
    return m_ok && m_st != nullptr;
}

bool EmpireMuster::update_pathing () {
    if (!ok() || m_st->m_player_states == nullptr) {
        return false;
    }
    if (m_st->m_player_states[m_seat].m_muster_city == U16_KEY_NULL) {
        return false;
    }
    return true;
}

u16 EmpireMuster::count_cap () const {
    if (!ok() || m_st->m_player_states == nullptr || m_st->m_statics == nullptr) {
        return 0u;
    }
    const u16 ci = m_st->m_player_states[m_seat].m_muster_city;
    if (ci == U16_KEY_NULL) {
        return 0u;
    }
    const City* c = m_st->m_cities.get_city(ci);
    if (c == nullptr || c->get_owner() != m_seat) {
        return 0u;
    }
    const u16 x = c->get_x();
    const u16 y = c->get_y();
    u16 n = 0u;
    u16 cur = m_st->m_map.get_unit_hd(x, y);
    while (cur != U16_KEY_NULL) {
        const UnitAddStruct* u = m_st->m_units.get_unit_add(UnitAddKey::from_raw(cur));
        if (u == nullptr) {
            break;
        }
        const u16 next_tile = u->m_next_unit_on_tile;
        if (u->m_player_idx == m_seat && u->m_health > 0u && u->m_in_campaign == 0u && u->m_in_army == 0u) {
            UnitAddKey g = UnitAddKey::from_raw(cur);
            while (g.is_valid()) {
                const UnitAddStruct* gu = m_st->m_units.get_unit_add(g);
                if (gu == nullptr) {
                    break;
                }
                if (gu->m_player_idx == m_seat && gu->m_health > 0u) {
                    const u16 ut = m_st->m_statics->unit().get_item(
                        UnitStaticDataKey::from_raw(gu->m_unit_typ_idx)).type;
                    if (UnitUtilityHelper::is_type_for_land_army(ut)) {
                        ++n;
                    }
                }
                if (gu->m_next_unit_in_group == U16_KEY_NULL) {
                    break;
                }
                g = UnitAddKey::from_raw(gu->m_next_unit_in_group);
            }
        }
        cur = next_tile;
    }
    return n;
}

bool EmpireMuster::check_dispatch () const {
    if (!ok() || m_disp.busy()) {
        return false;
    }
    return count_cap() >= s_min_n;
}

bool EmpireMuster::peel_cap (UnitAddKey* out_hd) {
    if (out_hd == nullptr || !ok() || m_st->m_player_states == nullptr) {
        return false;
    }
    const u16 ci = m_st->m_player_states[m_seat].m_muster_city;
    if (ci == U16_KEY_NULL) {
        return false;
    }
    const City* c = m_st->m_cities.get_city(ci);
    if (c == nullptr || c->get_owner() != m_seat) {
        return false;
    }
    UnitAddKey head = UnitAddKey::None();
    if (!UnitMovementMng::muster_leave_one_defense(*m_st, c->get_x(), c->get_y(), m_seat, &head)) {
        return false;
    }
    if (!head.is_valid()) {
        return false;
    }
    *out_hd = head;
    return true;
}

bool EmpireMuster::try_dispatch () {
    if (!check_dispatch()) {
        return false;
    }
    if (!m_disp.ok()) {
        return false;
    }
    UnitAddKey hd = UnitAddKey::None();
    if (!peel_cap(&hd)) {
        return false;
    }
    UnitMovementMng::set_grp_campaign(*m_st, hd, 1u);
    return m_disp.take_hd(hd);
}

bool EmpireMuster::set_front_dest (u16 dx, u16 dy) {
    if (!ok()) {
        return false;
    }
    return m_disp.set_dest(*m_st, m_seat, dx, dy);
}

MusterDispatch& EmpireMuster::disp () {
    return m_disp;
}

const MusterDispatch& EmpireMuster::disp () const {
    return m_disp;
}

u16 EmpireMuster::seat () const {
    return m_seat;
}

u16 EmpireMuster::pt_n () const {
    return m_pt_n;
}

u16 EmpireMuster::disp_n () const {
    return m_disp_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
