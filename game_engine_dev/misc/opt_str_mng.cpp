//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "opt_str_mng.h"

//================================================================================================================================
//=> - StringManager implementation -
//================================================================================================================================

StringManager::StringManager() : m_buf(0), m_buf_sz(0), m_idx(0), m_idx_n(0) {}
StringManager::~StringManager() { rst(); }

bool StringManager::load_file_content(cstr file_path) {
    rst();
    if (!file_path) {
        return false;
    }
    FILE* ptr = std::fopen(file_path, "rb");
    if (!ptr) {
        return false;
    }
    if (std::fseek(ptr, 0, SEEK_END) != 0) {
        std::fclose(ptr);
        return false;
    }
    long file_size = std::ftell(ptr);
    if (file_size < 0) {
        std::fclose(ptr);
        return false;
    }
    if (std::fseek(ptr, 0, SEEK_SET) != 0) {
        std::fclose(ptr);
        return false;
    }
    m_buf_sz = (u32)file_size;
    m_buf = new char[(std::size_t)m_buf_sz + 1];
    std::size_t read_n = 0;
    if (m_buf_sz > 0) {
        read_n = std::fread(m_buf, 1, (std::size_t)m_buf_sz, ptr);
    }
    std::fclose(ptr);
    if (m_buf_sz > 0 && read_n != (std::size_t)m_buf_sz) {
        rst();
        return false;
    }
    m_buf[m_buf_sz] = '\0';
    m_idx = new u32[1];
    m_idx[0] = 0;
    m_idx_n = 1;
    return true;
}

bool StringManager::load_cstr_content(cstr content) {
    rst();
    if (!content) {
        return false;
    }
    m_buf_sz = (u32)std::strlen(content);
    m_buf = new char[(std::size_t)m_buf_sz + 1];
    if (m_buf_sz > 0) {
        std::memcpy(m_buf, content, (std::size_t)m_buf_sz);
    }
    m_buf[m_buf_sz] = '\0';
    m_idx = new u32[1];
    m_idx[0] = 0;
    m_idx_n = 1;
    return true;
}

void StringManager::split_string_by_char(u32 string_index, char split_char) {
    if (!m_buf || !m_idx || m_buf_sz == 0 || string_index >= m_idx_n) {
        return;
    }
    u32 start = m_idx[string_index];
    u32 end = start;
    while (end < m_buf_sz && m_buf[end] != '\0') ++end;
    u32 split_count = 0;
    for (u32 i = start; i < end; ++i) {
        if (m_buf[i] == split_char) ++split_count;
    }
    if (split_count == 0) {
        return;
    }
    u32 new_count = m_idx_n + split_count;
    u32* ni = new u32[new_count];
    u32 k = 0;
    for (u32 i = 0; i <= string_index; ++i) {
        ni[k++] = m_idx[i];
    }
    for (u32 i = start; i < end; ++i) {
        if (m_buf[i] != split_char) {
            continue;
        }
        m_buf[i] = '\0';
        ni[k++] = i + 1;
    }
    for (u32 i = string_index + 1; i < m_idx_n; ++i) {
        ni[k++] = m_idx[i];
    }
    delete[] m_idx;
    m_idx = ni;
    m_idx_n = k;
}

void StringManager::trim_head_char(u32 string_index, char trim_char) {
    if (!m_buf || !m_idx || string_index >= m_idx_n) {
        return;
    }
    u32 b = m_idx[string_index];
    while (b < m_buf_sz && m_buf[b] == trim_char) {
        ++b;
    }
    m_idx[string_index] = b;
}

void StringManager::trim_tail_char(u32 string_index, char trim_char) {
    if (!m_buf || !m_idx || string_index >= m_idx_n) {
        return;
    }
    u32 b = m_idx[string_index];
    u32 e = b;
    while (e < m_buf_sz && m_buf[e] != '\0') {
        ++e;
    }
    while (e > b && m_buf[e - 1] == trim_char) {
        m_buf[e - 1] = '\0';
        --e;
    }
}

void StringManager::cull_empty_strings() {
    if (!m_buf || !m_idx) {
        return;
    }
    u32 keep_n = 0;
    for (u32 i = 0; i < m_idx_n; ++i) {
        u32 b = m_idx[i];
        if (b >= m_buf_sz) {
            continue;
        }
        if (m_buf[b] != '\0') {
            ++keep_n;
        }
    }
    if (keep_n == m_idx_n) {
        return;
    }
    if (keep_n == 0) {
        delete[] m_idx;
        m_idx = 0;
        m_idx_n = 0;
        return;
    }
    u32* ni = new u32[keep_n];
    u32 k = 0;
    for (u32 i = 0; i < m_idx_n; ++i) {
        u32 b = m_idx[i];
        if (b >= m_buf_sz) {
            continue;
        }
        if (m_buf[b] != '\0') {
            ni[k++] = b;
        }
    }
    delete[] m_idx;
    m_idx = ni;
    m_idx_n = k;
}

cstr StringManager::get_string_content(u32 string_index) const {
    if (!m_buf || !m_idx || string_index >= m_idx_n) {
        return "";
    }
    return m_buf + m_idx[string_index];
}

u32 StringManager::get_string_count() const {
    return m_idx_n;
}

void StringManager::rst() {
    delete[] m_buf;
    delete[] m_idx;
    m_buf = 0;
    m_idx = 0;
    m_buf_sz = 0;
    m_idx_n = 0;
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
