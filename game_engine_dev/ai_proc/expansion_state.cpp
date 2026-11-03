//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "expansion_state.h"

//================================================================================================================================
//=> - ExpansionState -
//================================================================================================================================

ExpansionState::ExpansionState () :
    m_seen(nullptr),
    m_path(nullptr),
    m_sec_n(0u),
    m_path_n(0u),
    m_start(U16_KEY_NULL),
    m_cur(U16_KEY_NULL),
    m_ox(0u),
    m_oy(0u),
    m_rs(1u),
    m_first(true),
    m_have(false),
    m_ok(false) {
}

ExpansionState::~ExpansionState () {
    clr();
}

void ExpansionState::clr () {
    delete m_seen;
    delete[] m_path;
    m_seen = nullptr;
    m_path = nullptr;
    m_sec_n = 0u;
    m_path_n = 0u;
    m_start = U16_KEY_NULL;
    m_cur = U16_KEY_NULL;
    m_ox = 0u;
    m_oy = 0u;
    m_rs = 1u;
    m_first = true;
    m_have = false;
    m_ok = false;
}

bool ExpansionState::begin (u16 sec_n) {
    clr();
    if (sec_n == 0u) {
        return false;
    }
    m_seen = new BitArrayCL(static_cast<u32>(sec_n));
    m_path = new u16[sec_n];
    if (m_seen == nullptr || m_path == nullptr) {
        clr();
        return false;
    }
    m_sec_n = sec_n;
    m_ok = true;
    reset_walk();
    return true;
}

void ExpansionState::reset_walk () {
    if (!m_ok || m_seen == nullptr) {
        return;
    }
    m_seen->clear_all();
    m_path_n = 0u;
    m_start = U16_KEY_NULL;
    m_cur = U16_KEY_NULL;
    m_ox = 0u;
    m_oy = 0u;
    m_rs = 1u;
    m_first = true;
    m_have = false;
}

bool ExpansionState::ok () const {
    return m_ok;
}

u16 ExpansionState::sec_n () const {
    return m_sec_n;
}

bool ExpansionState::visited (u16 s) const {
    return m_ok && m_seen != nullptr && s < m_sec_n && m_seen->get_bit(static_cast<u32>(s)) != 0u;
}

void ExpansionState::mark (u16 s) {
    if (!m_ok || m_seen == nullptr || s >= m_sec_n) {
        return;
    }
    m_seen->set_bit(static_cast<u32>(s));
}

bool ExpansionState::path_push (u16 s) {
    if (!m_ok || m_path == nullptr || m_path_n >= m_sec_n) {
        return false;
    }
    m_path[m_path_n] = s;
    m_path_n = static_cast<u16>(m_path_n + 1u);
    m_cur = s;
    return true;
}

bool ExpansionState::path_pop () {
    if (!m_ok || m_path_n <= 1u) {
        return false;
    }
    m_path_n = static_cast<u16>(m_path_n - 1u);
    m_cur = m_path[m_path_n - 1u];
    return true;
}

u16 ExpansionState::path_n () const {
    return m_path_n;
}

u16 ExpansionState::path_at (u16 i) const {
    return (m_path != nullptr && i < m_path_n) ? m_path[i] : U16_KEY_NULL;
}

u16 ExpansionState::path_top () const {
    return (m_path != nullptr && m_path_n > 0u) ? m_path[m_path_n - 1u] : U16_KEY_NULL;
}

u16 ExpansionState::start () const {
    return m_start;
}

void ExpansionState::set_start (u16 s) {
    m_start = s;
}

u16 ExpansionState::cur () const {
    return m_cur;
}

void ExpansionState::set_cur (u16 s) {
    m_cur = s;
}

u16 ExpansionState::ox () const {
    return m_ox;
}

u16 ExpansionState::oy () const {
    return m_oy;
}

void ExpansionState::set_origin (u16 x, u16 y) {
    m_ox = x;
    m_oy = y;
}

u32& ExpansionState::rs () {
    return m_rs;
}

bool ExpansionState::first () const {
    return m_first;
}

void ExpansionState::set_first (bool v) {
    m_first = v;
}

bool ExpansionState::have () const {
    return m_have;
}

void ExpansionState::set_have (bool v) {
    m_have = v;
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
