//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "border_turn_handler.h"

#include "city.h"
#include "city_array.h"
#include "game_array_simple.h"

//================================================================================================================================
//=> - BorderTurnHandler -
//================================================================================================================================

BorderTurnHandler::BorderTurnHandler () :
    m_map(nullptr),
    m_cities(nullptr),
    m_cap(U16_KEY_NULL),
    m_front_n(0),
    m_ok(false) {
    for (u16 i = 0; i < k_front_cap; ++i) {
        m_front[i] = U16_KEY_NULL;
        m_fx[i] = U16_KEY_NULL;
        m_fy[i] = U16_KEY_NULL;
    }
}

BorderTurnHandler::~BorderTurnHandler () {
}

bool BorderTurnHandler::begin (const GameArraySimple& map, const CityArray& cities) {
    m_map = &map;
    m_cities = &cities;
    clr();
    m_ok = true;
    return m_ok;
}

void BorderTurnHandler::clr () {
    m_cap = U16_KEY_NULL;
    m_front_n = 0;
    for (u16 i = 0; i < k_front_cap; ++i) {
        m_front[i] = U16_KEY_NULL;
        m_fx[i] = U16_KEY_NULL;
        m_fy[i] = U16_KEY_NULL;
    }
}

bool BorderTurnHandler::ok () const {
    return m_ok;
}

u16 BorderTurnHandler::capital_city () const {
    return m_cap;
}

u16 BorderTurnHandler::front_n () const {
    return m_front_n;
}

u16 BorderTurnHandler::front_city (u16 i) const {
    return (i < m_front_n) ? m_front[i] : U16_KEY_NULL;
}

u16 BorderTurnHandler::front_x (u16 i) const {
    return (i < m_front_n) ? m_fx[i] : U16_KEY_NULL;
}

u16 BorderTurnHandler::front_y (u16 i) const {
    return (i < m_front_n) ? m_fy[i] : U16_KEY_NULL;
}

bool BorderTurnHandler::slot_idle (u16 v) {
    return v == k_front_idle;
}

bool BorderTurnHandler::slot_need (u16 v) {
    return v == k_front_need;
}

bool BorderTurnHandler::slot_city (u16 v) {
    return v != k_front_idle && v != k_front_need;
}

u16 BorderTurnHandler::count_need (const BorderFront* fronts, u16 n) const {
    if (fronts == nullptr || n == 0u) {
        return 0u;
    }
    u16 c = 0u;
    for (u16 i = 0; i < n; ++i) {
        if (slot_need(fronts[i].m_city)) {
            c = static_cast<u16>(c + 1u);
        }
    }
    return c;
}

void BorderTurnHandler::gather_used (const BorderFront* fronts, u16 n, u16* used, u16* used_n) const {
    if (used_n == nullptr) {
        return;
    }
    *used_n = 0u;
    if (fronts == nullptr || used == nullptr || n == 0u) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        if (!slot_city(fronts[i].m_city)) {
            continue;
        }
        used[*used_n] = fronts[i].m_city;
        *used_n = static_cast<u16>(*used_n + 1u);
    }
}

void BorderTurnHandler::fill_need (BorderFront* fronts, u16 n, const BorderFront* picks, u16 pick_n) {
    if (fronts == nullptr || picks == nullptr || pick_n == 0u || n == 0u) {
        return;
    }
    u16 pi = 0u;
    for (u16 i = 0; i < n && pi < pick_n; ++i) {
        if (!slot_need(fronts[i].m_city)) {
            continue;
        }
        fronts[i] = picks[pi];
        pi = static_cast<u16>(pi + 1u);
    }
}

void BorderTurnHandler::fallback_cap (BorderFront* fronts, u16 n, u16 cap) {
    if (fronts == nullptr || n == 0u || cap == U16_KEY_NULL || m_cities == nullptr) {
        return;
    }
    for (u16 i = 0; i < n; ++i) {
        if (slot_city(fronts[i].m_city)) {
            return;
        }
    }
    for (u16 i = 0; i < n; ++i) {
        if (slot_need(fronts[i].m_city)) {
            fronts[i].m_city = cap;
            fronts[i].m_pl = U16_KEY_NULL;
            return;
        }
    }
}

void BorderTurnHandler::sync_from (const BorderFront* fronts, u16 n) {
    m_front_n = 0u;
    for (u16 i = 0; i < k_front_cap; ++i) {
        m_front[i] = U16_KEY_NULL;
        m_fx[i] = U16_KEY_NULL;
        m_fy[i] = U16_KEY_NULL;
    }
    if (fronts == nullptr || m_cities == nullptr || n == 0u) {
        return;
    }
    for (u16 i = 0; i < n && m_front_n < k_front_cap; ++i) {
        if (!slot_city(fronts[i].m_city)) {
            continue;
        }
        const City* c = m_cities->get_city(fronts[i].m_city);
        if (c == nullptr) {
            continue;
        }
        m_front[m_front_n] = fronts[i].m_city;
        m_fx[m_front_n] = c->get_x();
        m_fy[m_front_n] = c->get_y();
        m_front_n = static_cast<u16>(m_front_n + 1u);
    }
}

//================================================================================================================================
//=> - Impl select -
//================================================================================================================================

#ifndef BORDER_TURN_HANDLER_IMPL
#define BORDER_TURN_HANDLER_IMPL "impl/border_turn_handler_impl_mk06.cpp"
#endif

#include BORDER_TURN_HANDLER_IMPL

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
