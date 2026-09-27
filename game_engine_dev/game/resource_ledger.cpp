//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "resource_ledger.h"

#include <cstdio>
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

bool ResourceLedger::wr (void* fp_raw) const {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    if (std::fwrite(&m_n, sizeof(m_n), 1, fp) != 1) {
        return false;
    }
    if (m_n == 0u) {
        return true;
    }
    if (m_v == nullptr) {
        return false;
    }
    return std::fwrite(m_v, sizeof(u16), static_cast<size_t>(m_n), fp) == static_cast<size_t>(m_n);
}

bool ResourceLedger::rd (void* fp_raw) {
    std::FILE* fp = static_cast<std::FILE*>(fp_raw);
    if (fp == nullptr) {
        return false;
    }
    u16 n = 0;
    if (std::fread(&n, sizeof(n), 1, fp) != 1) {
        return false;
    }
    if (!setup(n)) {
        return false;
    }
    if (n == 0u) {
        return true;
    }
    return std::fread(m_v, sizeof(u16), static_cast<size_t>(n), fp) == static_cast<size_t>(n);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
