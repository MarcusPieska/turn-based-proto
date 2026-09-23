//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "eval_need.h"

//================================================================================================================================
//=> - EvalNeed -
//================================================================================================================================

EvalNeed::EvalNeed ()
    : m_trace(0),
      m_save_seq(0),
      m_save_n(0) {
    for (u16 i = 0; i < SAVE_MAX; ++i) {
        m_save_turn[i] = 0;
    }
}

EvalNeed& EvalNeed::trace () {
    m_trace = 1;
    return *this;
}

LogNeedMask& EvalNeed::logs () {
    return m_logs;
}

const LogNeedMask& EvalNeed::logs () const {
    return m_logs;
}

EvalNeed& EvalNeed::save (u32 turn) {
    if (turn == 0 || m_save_n >= SAVE_MAX) {
        return *this;
    }
    for (u16 i = 0; i < m_save_n; ++i) {
        if (m_save_turn[i] == turn) {
            return *this;
        }
    }
    m_save_turn[m_save_n] = turn;
    m_save_n = static_cast<u16>(m_save_n + 1u);
    return *this;
}

EvalNeed& EvalNeed::save_seq () {
    m_save_seq = 1;
    return *this;
}

bool EvalNeed::want_trace () const {
    return m_trace != 0 || m_logs.any();
}

bool EvalNeed::want_logs () const {
    return m_logs.any();
}

bool EvalNeed::want_saves () const {
    return m_save_n != 0;
}

bool EvalNeed::want_save_seq () const {
    return m_save_seq != 0;
}

u16 EvalNeed::save_n () const {
    return m_save_n;
}

u32 EvalNeed::save_turn (u16 i) const {
    if (i >= m_save_n) {
        return 0;
    }
    return m_save_turn[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
