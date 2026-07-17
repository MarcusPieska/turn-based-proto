//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "city_network_router.h"

#include "city.h"
#include "city_array.h"
#include "city_network.h"

//================================================================================================================================
//=> - CityNetworkRouter -
//================================================================================================================================

CityNetworkRouter::CityNetworkRouter () :
    m_ok(false),
    m_n(0),
    m_goal(U16_KEY_NULL),
    m_net(nullptr),
    m_q(nullptr),
    m_next(nullptr),
    m_seen(nullptr),
    m_tick(0) {
}

CityNetworkRouter::~CityNetworkRouter () {
    clear();
}

void CityNetworkRouter::clear () {
    delete[] m_q;
    m_q = nullptr;
    delete[] m_next;
    m_next = nullptr;
    delete[] m_seen;
    m_seen = nullptr;
    m_net = nullptr;
    m_n = 0;
    m_goal = U16_KEY_NULL;
    m_tick = 0;
    m_ok = false;
}

bool CityNetworkRouter::begin (const CityNetwork& net) {
    clear();
    if (!net.is_valid() || net.cities() == nullptr || net.city_n() == 0) {
        return false;
    }
    const u16 n = net.city_n();
    m_q = new u16[n];
    m_next = new u16[n];
    m_seen = new u32[n];
    for (u16 i = 0; i < n; ++i) {
        m_seen[i] = 0;
        m_next[i] = U16_KEY_NULL;
    }
    m_n = n;
    m_net = &net;
    m_goal = U16_KEY_NULL;
    m_tick = 0;
    m_ok = true;
    return true;
}

void CityNetworkRouter::build_to (u16 to, u16 stop) const {
    const CityArray* cities = m_net->cities();
    ++m_tick;
    if (m_tick == 0) {
        for (u16 i = 0; i < m_n; ++i) {
            m_seen[i] = 0;
        }
        m_tick = 1;
    }
    for (u16 i = 0; i < m_n; ++i) {
        m_next[i] = U16_KEY_NULL;
    }
    m_seen[to] = m_tick;
    m_next[to] = to;
    if (stop == to) {
        m_goal = to;
        return;
    }
    u32 qh = 0;
    u32 qt = 0;
    m_q[qt++] = to;
    bool hit = false;
    while (qh < qt && !hit) {
        const u16 cur = m_q[qh++];
        const City* c = cities->get_city(cur);
        if (c == nullptr) {
            continue;
        }
        const CityNetLinks& L = c->links();
        const u16 slots[4] = {L.m_ne, L.m_nw, L.m_se, L.m_sw};
        for (u32 s = 0; s < 4u; ++s) {
            const u16 j = slots[s];
            if (j == U16_KEY_NULL || j >= m_n) {
                continue;
            }
            if (m_seen[j] == m_tick) {
                continue;
            }
            m_seen[j] = m_tick;
            m_next[j] = cur;
            m_q[qt++] = j;
            if (j == stop) {
                hit = true;
                break;
            }
        }
    }
    m_goal = to;
}

u16 CityNetworkRouter::next_hop (u16 from, u16 to) const {
    if (!m_ok || m_net == nullptr || m_next == nullptr) {
        return U16_KEY_NULL;
    }
    if (from >= m_n || to >= m_n) {
        return U16_KEY_NULL;
    }
    if (from == to) {
        return from;
    }
    if (to != m_goal || m_seen[from] != m_tick) {
        build_to(to, from);
    }
    return m_next[from];
}

bool CityNetworkRouter::is_valid () const {
    return m_ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
