//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstring>

#include "str_pool.h"

//================================================================================================================================
//=> - StringPool implementation -
//================================================================================================================================

StringPool::StringPool(u32 char_capacity, u16 string_capacity)
    : m_buf(0), m_buf_cap(char_capacity), m_buf_n(0), m_idx(0), m_idx_cap(string_capacity), m_idx_n(0) {
    if (m_buf_cap > 0) {
        m_buf = new char[(std::size_t)m_buf_cap];
        std::memset(m_buf, 0, (std::size_t)m_buf_cap);
    }
    if (m_idx_cap > 0) {
        m_idx = new u32[(std::size_t)m_idx_cap];
    }
}

StringPool::~StringPool() {
    clear();
}

bool StringPool::push_string(cstr source, u16* out_id) {
    if (!source || !out_id || !m_buf || !m_idx) {
        return false;
    }
    if (m_idx_n >= m_idx_cap) {
        return false;
    }
    u32 len = (u32)std::strlen(source) + 1;
    if (m_buf_n + len > m_buf_cap) {
        return false;
    }
    u32 off = m_buf_n;
    std::memcpy(m_buf + off, source, (std::size_t)len);
    m_idx[m_idx_n] = off;
    *out_id = m_idx_n;
    ++m_idx_n;
    m_buf_n += len;
    return true;
}

char* StringPool::get_string(u16 string_id) {
    if (!m_buf || !m_idx || string_id >= m_idx_n) {
        return 0;
    }
    return m_buf + m_idx[string_id];
}

cstr StringPool::get_string(u16 string_id) const {
    if (!m_buf || !m_idx || string_id >= m_idx_n) {
        return "";
    }
    return m_buf + m_idx[string_id];
}

u16 StringPool::get_string_count() const {
    return m_idx_n;
}

u16 StringPool::get_string_capacity() const {
    return m_idx_cap;
}

u32 StringPool::get_char_count() const {
    return m_buf_n;
}

u32 StringPool::get_char_capacity() const {
    return m_buf_cap;
}

void StringPool::clear() {
    delete[] m_buf;
    delete[] m_idx;
    m_buf = 0;
    m_idx = 0;
    m_buf_cap = 0;
    m_buf_n = 0;
    m_idx_cap = 0;
    m_idx_n = 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
