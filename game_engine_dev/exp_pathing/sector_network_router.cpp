//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "sector_network_router.h"

#include "sector_network.h"

//================================================================================================================================
//=> - SectorPath -
//================================================================================================================================

void SectorPath::set (u8 i, u8 v) {
    u8* p = &m_pack[i >> 1];
    if ((i & 1u) == 0) {
        *p = static_cast<u8>((*p & 0xf0u) | (v & 0x0fu));
    } else {
        *p = static_cast<u8>((*p & 0x0fu) | ((v & 0x0fu) << 4));
    }
}

u8 SectorPath::get (u8 i) const {
    const u8 b = m_pack[i >> 1];
    if ((i & 1u) == 0) {
        return static_cast<u8>(b & 0x0fu);
    }
    return static_cast<u8>(b >> 4);
}

//================================================================================================================================
//=> - SectorNetworkRouter -
//================================================================================================================================

SectorNetworkRouter::SectorNetworkRouter () :
    m_ok(false),
    m_n(0),
    m_net(nullptr),
    m_q(nullptr),
    m_next(nullptr),
    m_seen(nullptr),
    m_tick(0) {
}

SectorNetworkRouter::~SectorNetworkRouter () {
    clear();
}

void SectorNetworkRouter::clear () {
    delete[] m_q;
    m_q = nullptr;
    delete[] m_next;
    m_next = nullptr;
    delete[] m_seen;
    m_seen = nullptr;
    m_net = nullptr;
    m_n = 0;
    m_tick = 0;
    m_ok = false;
}

bool SectorNetworkRouter::begin (const SectorNetwork& net) {
    clear();
    if (!net.is_valid() || net.sector_n() == 0) {
        return false;
    }
    const u16 n = net.sector_n();
    m_q = new u16[n];
    m_next = new u16[n];
    m_seen = new u32[n];
    for (u16 i = 0; i < n; ++i) {
        m_seen[i] = 0;
        m_next[i] = U16_KEY_NULL;
    }
    m_n = n;
    m_net = &net;
    m_tick = 0;
    m_ok = true;
    return true;
}

u8 SectorNetworkRouter::bit_to (const SectorNetwork& net, u16 from, u16 nxt) {
    for (u8 b = 0; b < 6u; ++b) {
        if (net.nbr(from, b) == nxt) {
            return b;
        }
    }
    return 255;
}

bool SectorNetworkRouter::build_to (u16 to, u16 stop) const {
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
        return true;
    }
    u32 qh = 0;
    u32 qt = 0;
    m_q[qt++] = to;
    while (qh < qt) {
        const u16 cur = m_q[qh++];
        for (u8 b = 0; b < 6u; ++b) {
            const u16 j = m_net->nbr(cur, b);
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
                return true;
            }
        }
    }
    return false;
}

bool SectorNetworkRouter::find (u16 from, u16 to, SectorPath* out) const {
    if (out == nullptr || !m_ok || m_net == nullptr || m_next == nullptr) {
        return false;
    }
    out->m_n = 0;
    if (from >= m_n || to >= m_n) {
        return false;
    }
    if (from == to) {
        return true;
    }
    if (!build_to(to, from)) {
        return false;
    }
    if (m_seen[from] != m_tick || m_next[from] == U16_KEY_NULL) {
        return false;
    }
    u16 cur = from;
    while (cur != to) {
        const u16 nxt = m_next[cur];
        if (nxt == U16_KEY_NULL || nxt == cur) {
            out->m_n = 0;
            return false;
        }
        const u8 bit = bit_to(*m_net, cur, nxt);
        if (bit > 5u || out->m_n >= SN_PATH_MAX) {
            out->m_n = 0;
            return false;
        }
        out->set(out->m_n, bit);
        ++out->m_n;
        cur = nxt;
    }
    return true;
}

bool SectorNetworkRouter::is_valid () const {
    return m_ok;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
