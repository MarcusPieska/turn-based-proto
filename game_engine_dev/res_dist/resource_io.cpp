//================================================================================================================================
//=> - Includes and globals -
//================================================================================================================================

#include <cstdio>
#include <cstring>

#include "resource_io.h"

//================================================================================================================================
//=> - ResIoPath -
//================================================================================================================================

ResIoPath::ResIoPath (cstr root_off) {
    m_root[0] = '\0';
    m_res[0] = '\0';
    if (root_off == nullptr) {
        return;
    }
    std::snprintf(m_root, sizeof(m_root), "%s", root_off);
    const size_t n = std::strlen(m_root);
    if (n > 0 && m_root[n - 1] != '/') {
        std::snprintf(m_res, sizeof(m_res), "%s/game_config.resources", m_root);
    } else {
        std::snprintf(m_res, sizeof(m_res), "%sgame_config.resources", m_root);
    }
}

cstr ResIoPath::resources_path () const {
    return m_res;
}

//================================================================================================================================
//=> - ResIoFile -
//================================================================================================================================

ResIoFile::ResIoFile () : m_line_n(0) {
}

ResIoFile::~ResIoFile () {
    rst();
}

void ResIoFile::rst () {
    m_line_n = 0;
}

bool ResIoFile::load (cstr path) {
    rst();
    if (path == nullptr) {
        return false;
    }
    if (!m_raw.load_file_content(path)) {
        return false;
    }
    if (!m_lines.load_cstr_content(m_raw.get_string_content(0))) {
        return false;
    }
    m_lines.split_string_by_char(0, '\n');
    m_lines.cull_empty_strings();
    m_line_n = m_lines.get_string_count();
    return m_line_n > 0;
}

u32 ResIoFile::line_n () const {
    return m_line_n;
}

cstr ResIoFile::line (u32 idx) const {
    if (idx >= m_line_n) {
        return nullptr;
    }
    return m_lines.get_string_content(idx);
}

//================================================================================================================================
//=> - End -
//================================================================================================================================
