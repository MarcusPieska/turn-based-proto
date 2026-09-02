//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include <cstring>

#include "trait_affinity_map.h"

//================================================================================================================================
//=> - TraitAffinityMap -
//================================================================================================================================

TraitAffinityMap::~TraitAffinityMap () {
    release_rows();
}

void TraitAffinityMap::set_rows (TraitAffinityRowStruct* rows, u16 row_n, StaticStringPool pool) {
    release_rows();
    m_rows = rows;
    m_row_n = row_n;
    m_owns_rows = false;
    m_pool = pool;
}

void TraitAffinityMap::adopt (TraitAffinityMap& src) {
    release_rows();
    m_rows = src.m_rows;
    m_row_n = src.m_row_n;
    m_owns_rows = src.m_owns_rows;
    m_pool = src.m_pool;
    src.m_rows = nullptr;
    src.m_row_n = 0;
    src.m_owns_rows = false;
    src.m_pool.reset();
}

void TraitAffinityMap::take_ownership () {
    if (m_owns_rows || m_rows == nullptr || m_row_n == 0) {
        return;
    }
    TraitAffinityRowStruct* tmp_rows = m_rows;
    StaticStringPool tmp_pool(m_pool);
    m_rows = new TraitAffinityRowStruct[m_row_n];
    for (u16 i = 0; i < m_row_n; ++i) {
        m_rows[i] = tmp_rows[i];
    }
    m_pool = tmp_pool;
    delete[] tmp_rows;
    m_owns_rows = true;
}

void TraitAffinityMap::release_rows () {
    if (m_owns_rows) {
        delete[] m_rows;
    }
    m_rows = nullptr;
    m_row_n = 0;
    m_owns_rows = false;
    m_pool.reset();
}

u16 TraitAffinityMap::get_row_count () const {
    return m_row_n;
}

u16 TraitAffinityMap::name_to_idx (cstr token) const {
    if (!token || token[0] == '\0') {
        return U16_KEY_NULL;
    }
    const u16 n = m_pool.get_str_n();
    for (u16 i = 0; i < n; ++i) {
        cstr nm = m_pool.get(i);
        if (nm && std::strcmp(nm, token) == 0) {
            return i;
        }
    }
    return U16_KEY_NULL;
}

cstr TraitAffinityMap::get_token_name (u16 idx) const {
    return m_pool.get(idx);
}

const TraitAffinityRowStruct& TraitAffinityMap::get_row (u16 idx) const {
    return m_rows[idx];
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
