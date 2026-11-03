//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "land_sector_adj.h"

//================================================================================================================================
//=> - LandSectorAdj -
//================================================================================================================================

LandSectorAdj::LandSectorAdj () :
    m_head(nullptr),
    m_to(nullptr),
    m_nxt(nullptr),
    m_len(nullptr),
    m_sec_n(0u),
    m_ok(false) {
}

LandSectorAdj::~LandSectorAdj () {
    clr();
}

void LandSectorAdj::clr () {
    delete[] m_head;
    delete[] m_to;
    delete[] m_nxt;
    delete[] m_len;
    m_head = nullptr;
    m_to = nullptr;
    m_nxt = nullptr;
    m_len = nullptr;
    m_sec_n = 0u;
    m_ok = false;
}

bool LandSectorAdj::ok () const {
    return m_ok;
}

u16 LandSectorAdj::sec_n () const {
    return m_sec_n;
}

u16 LandSectorAdj::hd (u16 s) const {
    return (m_ok && m_head != nullptr && s < m_sec_n) ? m_head[s] : U16_KEY_NULL;
}

u16 LandSectorAdj::to (u16 e) const {
    return (m_ok && m_to != nullptr) ? m_to[e] : U16_KEY_NULL;
}

u16 LandSectorAdj::nx (u16 e) const {
    return (m_ok && m_nxt != nullptr) ? m_nxt[e] : U16_KEY_NULL;
}

u16 LandSectorAdj::ln (u16 e) const {
    return (m_ok && m_len != nullptr) ? m_len[e] : 0u;
}

bool LandSectorAdj::build (const LandSectorNetwork& net, u16 sec_n) {
    clr();
    if (!net.ok() || sec_n == 0u) {
        return false;
    }
    const u32 edge_cap = static_cast<u32>(net.link_n()) * 2u + 1u;
    m_head = new u16[sec_n];
    m_to = new u16[edge_cap];
    m_nxt = new u16[edge_cap];
    m_len = new u16[edge_cap];
    if (m_head == nullptr || m_to == nullptr || m_nxt == nullptr || m_len == nullptr) {
        clr();
        return false;
    }
    m_sec_n = sec_n;
    for (u16 i = 0; i < sec_n; ++i) {
        m_head[i] = U16_KEY_NULL;
    }
    u16 en = 0u;
    for (u16 i = 0; i < net.link_n(); ++i) {
        const LandSectorLink* L = net.get(i);
        if (L == nullptr || L->m_a >= sec_n || L->m_b >= sec_n) {
            continue;
        }
        m_nxt[en] = m_head[L->m_a];
        m_to[en] = L->m_b;
        m_len[en] = L->m_len;
        m_head[L->m_a] = en;
        en = static_cast<u16>(en + 1u);
        m_nxt[en] = m_head[L->m_b];
        m_to[en] = L->m_a;
        m_len[en] = L->m_len;
        m_head[L->m_b] = en;
        en = static_cast<u16>(en + 1u);
    }
    m_ok = true;
    return true;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
