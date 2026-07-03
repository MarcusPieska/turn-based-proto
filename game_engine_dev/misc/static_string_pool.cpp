//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstring>

#include "static_string_pool.h"

//================================================================================================================================
//=> - StaticStringPool -
//================================================================================================================================

StaticStringPool::StaticStringPool () :
    m_buf(0),
    m_buf_cap(0),
    m_buf_n(0),
    m_off(0),
    m_off_cap(0),
    m_off_n(0) {
}

StaticStringPool::StaticStringPool (u16 str_cap, u32 char_n) :
    m_buf(0),
    m_buf_cap(char_n + (u32)str_cap),
    m_buf_n(0),
    m_off(0),
    m_off_cap(str_cap),
    m_off_n(0) {
    if (m_buf_cap > 0) {
        m_buf = new char[(std::size_t)m_buf_cap];
        std::memset(m_buf, 0, (std::size_t)m_buf_cap);
    }
    if (m_off_cap > 0) {
        m_off = new u32[(std::size_t)m_off_cap];
    }
}

StaticStringPool::~StaticStringPool () {
    clr();
}

StaticStringPool::StaticStringPool (const StaticStringPool& o) :
    m_buf(0),
    m_buf_cap(o.m_buf_cap),
    m_buf_n(o.m_buf_n),
    m_off(0),
    m_off_cap(o.m_off_cap),
    m_off_n(o.m_off_n) {
    if (m_buf_cap > 0) {
        m_buf = new char[(std::size_t)m_buf_cap];
        if (o.m_buf) {
            std::memcpy(m_buf, o.m_buf, (std::size_t)m_buf_cap);
        } else {
            std::memset(m_buf, 0, (std::size_t)m_buf_cap);
        }
    }
    if (m_off_cap > 0) {
        m_off = new u32[(std::size_t)m_off_cap];
        if (o.m_off) {
            std::memcpy(m_off, o.m_off, (std::size_t)m_off_cap * sizeof(u32));
        }
    }
}

StaticStringPool& StaticStringPool::operator= (const StaticStringPool& o) {
    if (this == &o) {
        return *this;
    }
    clr();
    m_buf_cap = o.m_buf_cap;
    m_buf_n = o.m_buf_n;
    m_off_cap = o.m_off_cap;
    m_off_n = o.m_off_n;
    if (m_buf_cap > 0) {
        m_buf = new char[(std::size_t)m_buf_cap];
        if (o.m_buf) {
            std::memcpy(m_buf, o.m_buf, (std::size_t)m_buf_cap);
        } else {
            std::memset(m_buf, 0, (std::size_t)m_buf_cap);
        }
    }
    if (m_off_cap > 0) {
        m_off = new u32[(std::size_t)m_off_cap];
        if (o.m_off) {
            std::memcpy(m_off, o.m_off, (std::size_t)m_off_cap * sizeof(u32));
        }
    }
    return *this;
}

bool StaticStringPool::add (cstr s, u16* out_k) {
    if (!s || !out_k || !m_buf || !m_off) {
        return false;
    }
    if (m_off_n >= m_off_cap) {
        return false;
    }
    u32 len = (u32)std::strlen(s) + 1u;
    if (m_buf_n + len > m_buf_cap) {
        return false;
    }
    u32 off = m_buf_n;
    std::memcpy(m_buf + off, s, (std::size_t)len);
    m_off[m_off_n] = off;
    *out_k = m_off_n;
    ++m_off_n;
    m_buf_n += len;
    return true;
}

cstr StaticStringPool::get (u16 k) const {
    if (!m_buf || !m_off || k >= m_off_n) {
        return "";
    }
    return m_buf + m_off[k];
}

u16 StaticStringPool::get_str_n () const {
    return m_off_n;
}

u16 StaticStringPool::get_str_cap () const {
    return m_off_cap;
}

u32 StaticStringPool::get_char_n () const {
    return m_buf_n;
}

u32 StaticStringPool::get_char_cap () const {
    return m_buf_cap;
}

void StaticStringPool::reset () {
    clr();
}

void StaticStringPool::clr () {
    delete[] m_buf;
    delete[] m_off;
    m_buf = 0;
    m_off = 0;
    m_buf_cap = 0;
    m_buf_n = 0;
    m_off_cap = 0;
    m_off_n = 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
