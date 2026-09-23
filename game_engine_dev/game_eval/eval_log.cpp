//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_log.h"

#include "eval_log_includes.inc"

#include <cstdio>
#include <cstring>

//================================================================================================================================
//=> - EvalLog -
//================================================================================================================================

EvalLog::EvalLog () : m_lines(nullptr), m_n(0), m_cap(0) {
}

EvalLog::~EvalLog () {
    clr();
}

void EvalLog::clr () {
    if (m_lines != nullptr) {
        for (u32 i = 0; i < m_n; ++i) {
            delete[] m_lines[i];
            m_lines[i] = nullptr;
        }
        delete[] m_lines;
        m_lines = nullptr;
    }
    m_n = 0;
    m_cap = 0;
}

bool EvalLog::load (cstr path) {
    clr();
    if (path == nullptr) {
        return false;
    }
    std::FILE* fp = std::fopen(path, "r");
    if (fp == nullptr) {
        return false;
    }
    char buf[2048];
    while (std::fgets(buf, sizeof(buf), fp) != nullptr) {
        u32 len = static_cast<u32>(std::strlen(buf));
        while (len > 0 && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) {
            buf[--len] = 0;
        }
        if (m_n == m_cap) {
            const u32 ncap = (m_cap == 0) ? 1024u : (m_cap * 2u);
            char** nl = new char*[ncap];
            for (u32 i = 0; i < m_n; ++i) {
                nl[i] = m_lines[i];
            }
            for (u32 i = m_n; i < ncap; ++i) {
                nl[i] = nullptr;
            }
            delete[] m_lines;
            m_lines = nl;
            m_cap = ncap;
        }
        char* row = new char[len + 1u];
        std::memcpy(row, buf, len + 1u);
        m_lines[m_n] = row;
        m_n = m_n + 1u;
    }
    std::fclose(fp);
    if (m_lines == nullptr) {
        m_lines = new char*[1];
        m_lines[0] = nullptr;
        m_cap = 1;
    }
    return true;
}

bool EvalLog::ok () const {
    return m_lines != nullptr;
}

u32 EvalLog::line_n () const {
    return m_n;
}

cstr EvalLog::line (u32 i) const {
    return m_lines[i];
}

u32 EvalLog::count_i (u16 kind_i) const {
    u32 n = 0;
    for (u32 li = 0; li < m_n; ++li) {
        const char* s = m_lines[li];
        switch (kind_i) {
#include "eval_log_count.inc"
        default:
            break;
        }
    }
    return n;
}

bool EvalLog::parse_tech_discover (u32 i, u16* player, u16* tech) const {
    if (i >= m_n || player == nullptr || tech == nullptr) {
        return false;
    }
    return LOG_PLAYER_TECH_DISCOVER::PARSE(m_lines[i], player, tech);
}

bool EvalLog::parse_city_foundation (u32 i, u16* x, u16* y, u16* player) const {
    if (i >= m_n || x == nullptr || y == nullptr || player == nullptr) {
        return false;
    }
    return LOG_CITY_FOUNDATION::PARSE(m_lines[i], x, y, player);
}

bool EvalLog::parse_new_turn (u32 i, u16* turn) const {
    if (i >= m_n || turn == nullptr) {
        return false;
    }
    return LOG_NEW_TURN::PARSE(m_lines[i], turn);
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
