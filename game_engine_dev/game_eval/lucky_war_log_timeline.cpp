//================================================================================================================================
//=> - Includes -
//================================================================================================================================

#include "lucky_war_log_timeline.h"

//================================================================================================================================
//=> - LuckyWarLogTimeline -
//================================================================================================================================

LuckyWarLogTimeline::LuckyWarLogTimeline () : m_n(0u) {
}

void LuckyWarLogTimeline::clr () {
    m_n = 0u;
}

bool LuckyWarLogTimeline::add (u16 turn, LuckyWarEv kind, u16 a, u16 b) {
    if (m_n >= k_cap) {
        return false;
    }
    m_ev[m_n].m_turn = turn;
    m_ev[m_n].m_kind = kind;
    m_ev[m_n].m_a = a;
    m_ev[m_n].m_b = b;
    m_n = static_cast<u16>(m_n + 1u);
    return true;
}

void LuckyWarLogTimeline::swap (LuckyWarLogTimeline& o) {
    const u16 n = (m_n > o.m_n) ? m_n : o.m_n;
    for (u16 i = 0; i < n; ++i) {
        const LuckyWarEvt t = m_ev[i];
        m_ev[i] = o.m_ev[i];
        o.m_ev[i] = t;
    }
    const u16 tn = m_n;
    m_n = o.m_n;
    o.m_n = tn;
}

u16 LuckyWarLogTimeline::n () const {
    return m_n;
}

const LuckyWarEvt& LuckyWarLogTimeline::at (u16 i) const {
    return m_ev[i];
}

//================================================================================================================================
//=> - End of file -
//================================================================================================================================
