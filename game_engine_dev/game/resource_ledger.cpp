//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "resource_ledger.h"

#include <cstring>

//================================================================================================================================
//=> - ResourceLedger -
//================================================================================================================================

ResourceLedger::ResourceLedger () : m_v(nullptr), m_n(0) {
}

ResourceLedger::~ResourceLedger () {
    clear();
}

bool ResourceLedger::setup (u16 res_n) {
    clear();
    if (res_n == 0) {
        return true;
    }
    m_v = new u16[res_n];
    if (m_v == nullptr) {
        return false;
    }
    std::memset(m_v, 0, sizeof(u16) * static_cast<size_t>(res_n));
    m_n = res_n;
    return true;
}

void ResourceLedger::clear () {
    delete[] m_v;
    m_v = nullptr;
    m_n = 0;
}

bool ResourceLedger::add (u16 res_idx, u16 amt) {
    if (m_v == nullptr || res_idx >= m_n || amt == 0) {
        return false;
    }
    const u32 next = static_cast<u32>(m_v[res_idx]) + static_cast<u32>(amt);
    m_v[res_idx] = next > CAP ? CAP : static_cast<u16>(next);
    return true;
}

u16 ResourceLedger::get (u16 res_idx) const {
    if (m_v == nullptr || res_idx >= m_n) {
        return 0;
    }
    return m_v[res_idx];
}

u16 ResourceLedger::count () const {
    return m_n;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
